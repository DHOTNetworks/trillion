/* MDB Tools - A library for reading MS Access database file
 * Copyright (C) 2000 Brian Bruns
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mdbtools.h"

static gint32
mdb_map_find_next0(MdbHandle *mdb, unsigned char *map, unsigned int map_sz, guint32 start_pg)
{
	guint32 pgnum, i, usage_bitlen;
	unsigned char *usage_bitmap;

	if (map_sz < 5) {
		return 0;
	}

	pgnum = mdb_get_int32(map, 1);
	usage_bitmap = map + 5;
	usage_bitlen = (map_sz - 5) * 8;

	i = (start_pg >= pgnum) ? start_pg-pgnum+1 : 0;
	for (; i<usage_bitlen; i++) {
		if (usage_bitmap[i/8] & (1 << (i%8))) {
			return pgnum + i;
		}
	}
	/* didn't find anything */
	return 0;
}
static gint32
mdb_map_find_next1(MdbHandle *mdb, unsigned char *map, unsigned int map_sz, guint32 start_pg)
{
	guint32 map_ind, max_map_pgs, offset, usage_bitlen;

	/*
	* start_pg will tell us where to (re)start the scan
	* for the next data page.  each usage_map entry points to a
	* 0x05 page which bitmaps (mdb->fmt->pg_size - 4) * 8 pages.
	*
	* map_ind gives us the starting usage_map entry
	* offset gives us a page offset into the bitmap
	*/
	usage_bitlen = (mdb->fmt->pg_size - 4) * 8;
	max_map_pgs = (map_sz - 1) / 4;
	map_ind = (start_pg + 1) / usage_bitlen;
	offset = (start_pg + 1) % usage_bitlen;

	for (; map_ind<max_map_pgs; map_ind++) {
		unsigned char *usage_bitmap;
		guint32 i, map_pg;

		if (!(map_pg = mdb_get_int32(map, (map_ind*4)+1))) {
			continue;
		}
		if(mdb_read_alt_pg(mdb, map_pg) != mdb->fmt->pg_size) {
			fprintf(stderr, "Oops! didn't get a full page at %d\n", map_pg);
			return -1;
		} 

		usage_bitmap = mdb->alt_pg_buf + 4;
		for (i=offset; i<usage_bitlen; i++) {
			if (usage_bitmap[i/8] & (1 << (i%8))) {
				return map_ind*usage_bitlen + i;
			}
		}
		offset = 0;
	}
	/* didn't find anything */
	return 0;
}

/* returns 0 on EOF */
/* returns -1 on error (unsupported map type) */
gint32
mdb_map_find_next(MdbHandle *mdb, unsigned char *map, unsigned int map_sz, guint32 start_pg)
{
	if (map[0] == 0) {
		return mdb_map_find_next0(mdb, map, map_sz, start_pg);
	} else if (map[0] == 1) {
		return mdb_map_find_next1(mdb, map, map_sz, start_pg);
	}

	fprintf(stderr, "Warning: unrecognized usage map type: %d\n", map[0]);
	return -1;
}
static void
mdb_map_set_bit(MdbHandle *mdb, unsigned char *map, unsigned int map_sz, guint32 pgnum)
{
	if (!map || map_sz < 5) return;

	if (map[0] == 0) {
		guint32 start_pg = mdb_get_int32(map, 1);
		if (pgnum >= start_pg) {
			guint32 bit_idx = pgnum - start_pg;
			if (bit_idx < (map_sz - 5) * 8) {
				map[5 + bit_idx / 8] |= (1 << (bit_idx % 8));
			}
		}
	} else if (map[0] == 1) {
		guint32 usage_bitlen = (mdb->fmt->pg_size - 4) * 8;
		guint32 max_map_pgs = (map_sz - 1) / 4;
		guint32 map_ind = pgnum / usage_bitlen;
		guint32 offset = pgnum % usage_bitlen;

		if (map_ind < max_map_pgs) {
			guint32 map_pg = mdb_get_int32(map, (map_ind * 4) + 1);
			if (map_pg > 0) {
				void *bm_buf = g_malloc(mdb->fmt->pg_size);
				if (fseeko(mdb->f->stream, (off_t)map_pg * mdb->fmt->pg_size, SEEK_SET) == 0 &&
				    fread(bm_buf, 1, mdb->fmt->pg_size, mdb->f->stream) == (size_t)mdb->fmt->pg_size) {
					unsigned char *usage_bitmap = (unsigned char *)bm_buf + 4;
					usage_bitmap[offset / 8] |= (1 << (offset % 8));
					fseeko(mdb->f->stream, (off_t)map_pg * mdb->fmt->pg_size, SEEK_SET);
					fwrite(bm_buf, 1, mdb->fmt->pg_size, mdb->f->stream);
					fflush(mdb->f->stream);
				}
				g_free(bm_buf);
			}
		}
	}
}

void
mdb_map_sync_to_disk(MdbTableDef *table)
{
	MdbCatalogEntry *entry = table->entry;
	MdbHandle *mdb = entry->mdb;
	MdbFormatConstants *fmt = mdb->fmt;
	void *tab_buf;
	guint32 pg_row;

	if (!mdb->f || !mdb->f->stream || !mdb->f->writable || entry->table_pg == 0) {
		return;
	}

	tab_buf = g_malloc(fmt->pg_size);
	if (fseeko(mdb->f->stream, (off_t)entry->table_pg * fmt->pg_size, SEEK_SET) != 0 ||
	    fread(tab_buf, 1, fmt->pg_size, mdb->f->stream) != (size_t)fmt->pg_size) {
		g_free(tab_buf);
		return;
	}

	/* Sync usage map */
	if (table->usage_map && table->map_sz > 0) {
		pg_row = mdb_get_int32(tab_buf, fmt->tab_usage_map_offset);
		guint32 map_pg = pg_row >> 8;
		guint16 map_row = pg_row & 0xff;
		if (map_pg > 0 && map_pg == entry->table_pg) {
			int rco = fmt->row_count_offset;
			int nrows = mdb_get_int16(tab_buf, rco);
			if (map_row < nrows) {
				int rstart = mdb_get_int16(tab_buf, (rco + 2) + map_row * 2) & 0x0FFF;
				memcpy((char*)tab_buf + rstart, table->usage_map, table->map_sz);
			}
		} else if (map_pg > 0) {
			void *map_buf = g_malloc(fmt->pg_size);
			if (fseeko(mdb->f->stream, (off_t)map_pg * fmt->pg_size, SEEK_SET) == 0 &&
			    fread(map_buf, 1, fmt->pg_size, mdb->f->stream) == (size_t)fmt->pg_size) {
				int rco = fmt->row_count_offset;
				int nrows = mdb_get_int16(map_buf, rco);
				if (map_row < nrows) {
					int rstart = mdb_get_int16(map_buf, (rco + 2) + map_row * 2) & 0x0FFF;
					memcpy((char*)map_buf + rstart, table->usage_map, table->map_sz);
					fseeko(mdb->f->stream, (off_t)map_pg * fmt->pg_size, SEEK_SET);
					fwrite(map_buf, 1, fmt->pg_size, mdb->f->stream);
				}
			}
			g_free(map_buf);
		}
	}

	/* Sync free usage map */
	if (table->free_usage_map && table->freemap_sz > 0) {
		pg_row = mdb_get_int32(tab_buf, fmt->tab_free_map_offset);
		guint32 map_pg = pg_row >> 8;
		guint16 map_row = pg_row & 0xff;
		if (map_pg > 0 && map_pg == entry->table_pg) {
			int rco = fmt->row_count_offset;
			int nrows = mdb_get_int16(tab_buf, rco);
			if (map_row < nrows) {
				int rstart = mdb_get_int16(tab_buf, (rco + 2) + map_row * 2) & 0x0FFF;
				memcpy((char*)tab_buf + rstart, table->free_usage_map, table->freemap_sz);
			}
		} else if (map_pg > 0) {
			void *map_buf = g_malloc(fmt->pg_size);
			if (fseeko(mdb->f->stream, (off_t)map_pg * fmt->pg_size, SEEK_SET) == 0 &&
			    fread(map_buf, 1, fmt->pg_size, mdb->f->stream) == (size_t)fmt->pg_size) {
				int rco = fmt->row_count_offset;
				int nrows = mdb_get_int16(map_buf, rco);
				if (map_row < nrows) {
					int rstart = mdb_get_int16(map_buf, (rco + 2) + map_row * 2) & 0x0FFF;
					memcpy((char*)map_buf + rstart, table->free_usage_map, table->freemap_sz);
					fseeko(mdb->f->stream, (off_t)map_pg * fmt->pg_size, SEEK_SET);
					fwrite(map_buf, 1, fmt->pg_size, mdb->f->stream);
				}
			}
			g_free(map_buf);
		}
	}

	/* Write back table header page */
	fseeko(mdb->f->stream, (off_t)entry->table_pg * fmt->pg_size, SEEK_SET);
	fwrite(tab_buf, 1, fmt->pg_size, mdb->f->stream);
	fflush(mdb->f->stream);
	g_free(tab_buf);
}

gint32
mdb_alloc_page(MdbTableDef *table)
{
	MdbCatalogEntry *entry = table->entry;
	MdbHandle *mdb = entry->mdb;
	MdbFormatConstants *fmt = mdb->fmt;
	void *new_pg;
	gint32 new_pgnum;
	off_t cur_len;

	if (!mdb->f || !mdb->f->stream || !mdb->f->writable) {
		return 0;
	}

	fseeko(mdb->f->stream, 0, SEEK_END);
	cur_len = ftello(mdb->f->stream);
	new_pgnum = cur_len / fmt->pg_size;

	new_pg = mdb_new_data_pg(entry);
	fseeko(mdb->f->stream, cur_len, SEEK_SET);
	if (fwrite(new_pg, 1, fmt->pg_size, mdb->f->stream) != (size_t)fmt->pg_size) {
		g_free(new_pg);
		return 0;
	}
	g_free(new_pg);
	fflush(mdb->f->stream);

	/* Update both Type 0 and Type 1 usage maps and free maps */
	mdb_map_set_bit(mdb, table->usage_map, table->map_sz, (guint32)new_pgnum);
	mdb_map_set_bit(mdb, table->free_usage_map, table->freemap_sz, (guint32)new_pgnum);

	mdb_map_sync_to_disk(table);
	mdb_read_pg(mdb, new_pgnum);

	return new_pgnum;
}

gint32
mdb_map_find_next_freepage(MdbTableDef *table, int row_size)
{
	MdbCatalogEntry *entry = table->entry;
	MdbHandle *mdb = entry->mdb;
	gint32 pgnum;
	guint32 cur_pg = (table->cur_phys_pg > 0) ? table->cur_phys_pg - 1 : 0;
	int free_space;

	/* 1. Fast path: check current active page for this table first */
	if (table->cur_phys_pg > 0) {
		mdb_read_pg(mdb, table->cur_phys_pg);
		if (mdb->pg_buf[0] == MDB_PAGE_DATA && mdb_get_int32(mdb->pg_buf, 4) == (long)entry->table_pg) {
			free_space = mdb_pg_get_freespace(mdb);
			if (free_space >= row_size) {
				return table->cur_phys_pg;
			}
		}
	}

	while (1) {
		pgnum = mdb_map_find_next(mdb, 
				table->free_usage_map, 
				table->freemap_sz, cur_pg);
		if (!pgnum) {
			/* allocate new page */
			pgnum = mdb_alloc_page(table);
			table->cur_phys_pg = pgnum;
			return pgnum;
		} else if (pgnum == -1) {
			fprintf(stderr, "Error: mdb_map_find_next_freepage error while reading maps.\n");
			return -1;
		}
		cur_pg = pgnum;

		mdb_read_pg(mdb, pgnum);
		free_space = mdb_pg_get_freespace(mdb);
		if (free_space >= row_size) {
			table->cur_phys_pg = pgnum;
			return pgnum;
		}
	}

	table->cur_phys_pg = pgnum;
	return pgnum;
}
