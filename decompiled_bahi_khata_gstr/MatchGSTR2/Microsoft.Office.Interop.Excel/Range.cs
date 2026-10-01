using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Microsoft.Office.Interop.Excel;

[ComImport]
[TypeIdentifier]
[CompilerGenerated]
[InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
[Guid("00020846-0000-0000-C000-000000000046")]
public interface Range
{
	void _VtblGap1_15();

	[PreserveSig]
	[DispId(237)]
	[return: MarshalAs(UnmanagedType.Struct)]
	object AutoFit();

	void _VtblGap2_29();

	[IndexerName("_Default")]
	[DispId(0)]
	object this[[Optional][In][MarshalAs(UnmanagedType.Struct)] object RowIndex, [Optional][In][MarshalAs(UnmanagedType.Struct)] object ColumnIndex]
	{
		[PreserveSig]
		[DispId(0)]
		[return: MarshalAs(UnmanagedType.Struct)]
		get;
		[PreserveSig]
		[DispId(0)]
		[param: In]
		[param: MarshalAs(UnmanagedType.Struct)]
		set;
	}

	void _VtblGap3_66();

	[DispId(193)]
	object NumberFormat
	{
		[PreserveSig]
		[DispId(193)]
		[return: MarshalAs(UnmanagedType.Struct)]
		get;
		[PreserveSig]
		[DispId(193)]
		[param: In]
		[param: MarshalAs(UnmanagedType.Struct)]
		set;
	}
}
