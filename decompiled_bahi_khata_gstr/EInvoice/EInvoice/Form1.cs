using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Data.Common;
using System.Data.OleDb;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using EInvoice.My;
using Microsoft.Office.Interop.Excel;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.CompilerServices;
using Microsoft.VisualBasic.Devices;
using Microsoft.VisualBasic.FileIO;
using Newtonsoft.Json.Linq;

namespace EInvoice;

[DesignerGenerated]
public class Form1 : Form
{
	private string JpgImageName;

	private bool first_button_cliked;

	private long header_row;

	private long starting_row;

	private string EXCEL_path;

	private Dictionary<string, string> variable1;

	private string title_msg;

	private string only_excel_file_name;

	private string special_feild_name;

	private bool field_exists;

	private string save_file_nm;

	private string json_String;

	private bool Gen_JSON_File;

	private IContainer components;

	[field: AccessedThroughProperty("Button1")]
	internal virtual Button Button1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = Button1_Click;
			Button val = field;
			if (val != null)
			{
				((Control)val).Click -= eventHandler;
			}
			field = value;
			val = field;
			if (val != null)
			{
				((Control)val).Click += eventHandler;
			}
		}
	}

	[field: AccessedThroughProperty("Button2")]
	internal virtual Button Button2
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = Button2_Click;
			Button val = field;
			if (val != null)
			{
				((Control)val).Click -= eventHandler;
			}
			field = value;
			val = field;
			if (val != null)
			{
				((Control)val).Click += eventHandler;
			}
		}
	}

	[field: AccessedThroughProperty("Button3")]
	internal virtual Button Button3
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = Button3_Click;
			Button val = field;
			if (val != null)
			{
				((Control)val).Click -= eventHandler;
			}
			field = value;
			val = field;
			if (val != null)
			{
				((Control)val).Click += eventHandler;
			}
		}
	}

	[field: AccessedThroughProperty("PictureBox1")]
	internal virtual PictureBox PictureBox1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("Button4")]
	internal virtual Button Button4
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = Button4_Click;
			Button val = field;
			if (val != null)
			{
				((Control)val).Click -= eventHandler;
			}
			field = value;
			val = field;
			if (val != null)
			{
				((Control)val).Click += eventHandler;
			}
		}
	}

	[field: AccessedThroughProperty("lblIRNno")]
	internal virtual Label lblIRNno
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = Label1_Click;
			Label val = field;
			if (val != null)
			{
				((Control)val).Click -= eventHandler;
			}
			field = value;
			val = field;
			if (val != null)
			{
				((Control)val).Click += eventHandler;
			}
		}
	}

	[field: AccessedThroughProperty("SaveFileDialog1")]
	internal virtual SaveFileDialog SaveFileDialog1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("pb1")]
	internal virtual ProgressBar pb1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("Label1")]
	internal virtual Label Label1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = Label1_Click_1;
			Label val = field;
			if (val != null)
			{
				((Control)val).Click -= eventHandler;
			}
			field = value;
			val = field;
			if (val != null)
			{
				((Control)val).Click += eventHandler;
			}
		}
	}

	[field: AccessedThroughProperty("Button5")]
	internal virtual Button Button5
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = Button5_Click;
			Button val = field;
			if (val != null)
			{
				((Control)val).Click -= eventHandler;
			}
			field = value;
			val = field;
			if (val != null)
			{
				((Control)val).Click += eventHandler;
			}
		}
	}

	[field: AccessedThroughProperty("Button6")]
	internal virtual Button Button6
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = Button6_Click;
			Button val = field;
			if (val != null)
			{
				((Control)val).Click -= eventHandler;
			}
			field = value;
			val = field;
			if (val != null)
			{
				((Control)val).Click += eventHandler;
			}
		}
	}

	[field: AccessedThroughProperty("Button7")]
	internal virtual Button Button7
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = Button7_Click;
			Button val = field;
			if (val != null)
			{
				((Control)val).Click -= eventHandler;
			}
			field = value;
			val = field;
			if (val != null)
			{
				((Control)val).Click += eventHandler;
			}
		}
	}

	public Form1()
	{
		//IL_0022: Unknown result type (might be due to invalid IL or missing references)
		//IL_002c: Expected Obj, but got Unknown
		((Form)this).Load += Form1_Load;
		((Control)this).KeyDown += Form1_KeyDown;
		variable1 = new Dictionary<string, string>();
		Gen_JSON_File = false;
		InitializeComponent();
	}

	private void field_exists_or_not(string field_nm, string tbl_nm)
	{
		//IL_001e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0024: Expected Obj, but got Unknown
		int try0001_dispatch = -1;
		checked
		{
			int num2 = default;
			int num = default;
			while (true)
			{
				try
				{
					/*Note: ILSpy has introduced the following switch to emulate a goto from catch-block to try-block*/;
					switch (try0001_dispatch)
					{
					default:
					{
						ProjectData.ClearProjectError();
						num2 = 2;
						string text = "Select * from " + tbl_nm;
						OleDbDataAdapter val = new OleDbDataAdapter(text, Module1.con);
						DataSet dataSet = new DataSet();
						((DbDataAdapter)(object)val).Fill(dataSet);
						DataTable dataTable = dataSet.Tables[0];
						int num3 = dataTable.Columns.Count - 1;
						for (int i = 0; i <= num3; i++)
						{
							if (Operators.CompareString(Strings.UCase(dataTable.Columns[i].ColumnName), Strings.UCase(field_nm), TextCompare: false) == 0)
							{
								field_exists = true;
								goto end_IL_0001;
							}
						}
						break;
					}
					case 225:
						num = -1;
						switch (num2)
						{
						case 2:
							break;
						default:
							goto end_IL_0001_2;
						}
						break;
					}
					if (Operators.CompareString(Information.Err().Description, "", TextCompare: false) != 0)
					{
						Interaction.MsgBox(Information.Err().Description + "\r\n\r\n" + Module1.con.ConnectionString);
					}
					break;
					end_IL_0001_2:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 225;
					continue;
				}
				throw ProjectData.CreateProjectError(-2146828237);
				continue;
				end_IL_0001:
				break;
			}
			if (num != 0)
			{
				ProjectData.ClearProjectError();
			}
		}
	}

	private void Form1_Load(object sender, EventArgs e)
	{
		//IL_00e5: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ec: Expected Obj, but got Unknown
		int try0001_dispatch = -1;
		int num2 = default;
		int num = default;
		while (true)
		{
			try
			{
				/*Note: ILSpy has introduced the following switch to emulate a goto from catch-block to try-block*/;
				switch (try0001_dispatch)
				{
				default:
				{
					ProjectData.ClearProjectError();
					num2 = 2;
					lblIRNno.Text = "";
					Label1.Text = "";
					title_msg = "Kamra Softwares";
					((Form)this).KeyPreview = true;
					((Form)this).Text = "Kamra Softwares (E-Invoice System)";
					con_access();
					if (!Module1.with_args)
					{
						Module1.source_Excel_file = Application.StartupPath + "\\EXCEL_TEST.xlsm";
					}
					only_excel_file_name = Path.GetFileName(Module1.source_Excel_file);
					string text = "TempEInvoiceTable";
					string text2 = "srno";
					field_exists = false;
					field_exists_or_not(text2, text);
					if (!field_exists)
					{
						string text3 = "ALTER TABLE " + text + " ADD COLUMN " + text2 + " COUNTER PRIMARY KEY";
						OleDbCommand val = new OleDbCommand(text3, Module1.con);
						val.ExecuteNonQuery();
					}
					if (Operators.CompareString(Module1.FormOpen, "0", TextCompare: false) == 0)
					{
						((Form)this).Opacity = 0.0;
						((Control)this).Visible = false;
						Button7_Click(RuntimeHelpers.GetObjectValue(sender), e);
						Application.Exit();
					}
					break;
				}
				case 373:
					num = -1;
					switch (num2)
					{
					case 2:
						break;
					default:
						goto end_IL_0001;
					}
					break;
				}
				if (Operators.CompareString(Information.Err().Description, "", TextCompare: false) != 0)
				{
					Interaction.MsgBox(Information.Err().Description);
				}
				break;
				end_IL_0001:;
			}
			catch (Exception ex) when ((num2 != 0) & (num == 0))
			{
				ProjectData.SetProjectError(ex);
				try0001_dispatch = 373;
				continue;
			}
			throw ProjectData.CreateProjectError(-2146828237);
		}
		if (num != 0)
		{
			ProjectData.ClearProjectError();
		}
	}

	private void Button1_Click(object sender, EventArgs e)
	{
		Process.Start("https://einvoice1.gst.gov.in/");
	}

	private bool IsElementPresent(string by1)
	{
		bool result = default;
		return result;
	}

	private void Button2_Click(object sender, EventArgs e)
	{
	}

	private void GETBase64String(string str1)
	{
		string text = str1;
		long num = text.IndexOf("base64");
		text = checked(text.Substring((int)(num + 7), (int)Math.Round((double)text.Length - Conversion.Val(num + 7))));
		Bitmap imageFromBase = GetImageFromBase64(text);
		PictureBox1.Image = (Image)(object)imageFromBase;
	}

	private void Button3_Click(object sender, EventArgs e)
	{
		special_feild_name = "Other charges";
		if (Operators.CompareString(special_feild_name, "Other Charges", TextCompare: false) == 0)
		{
			Interaction.MsgBox("First");
		}
		else if (Operators.CompareString(special_feild_name, "Other charges", TextCompare: false) == 0)
		{
			Interaction.MsgBox("Second");
		}
	}

	private Bitmap GetImageFromBase64(object Base64String)
	{
		//IL_0030: Unknown result type (might be due to invalid IL or missing references)
		//IL_0036: Expected Obj, but got Unknown
		Bitmap val = default;
		if (Operators.ConditionalCompareObjectNotEqual(string.Empty, Base64String, TextCompare: false))
		{
			byte[] buffer = Convert.FromBase64String(Conversions.ToString(Base64String));
			using MemoryStream memoryStream = new MemoryStream(buffer);
			val = (Bitmap)Image.FromStream((Stream)memoryStream);
			if (!Information.IsNothing(val))
			{
				if (!Directory.Exists("c:\\Base64ImageViwer"))
				{
					Directory.CreateDirectory("c:\\Base64ImageViwer");
				}
				((Image)val).Save("c:\\Base64ImageViwer\\TempImg.jpg", ImageFormat.Jpeg);
				((Image)val).Save(JpgImageName, ImageFormat.Jpeg);
				PictureBox1.Image = (Image)(object)val;
			}
		}
		return val;
	}

	public void con_access()
	{
		if (Module1.con.State == ConnectionState.Open)
		{
			Module1.con.Close();
		}
		string startupPath = Application.StartupPath;
		string text = ((!Module1.with_args) ? Module1.db_path : Module1.db_path);
		Module1.con.ConnectionString = "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + text + " ;Jet OLEDB:Database Password=jay_hanuman;";
		Module1.con.Open();
	}

	private void Button4_Click(object sender, EventArgs e)
	{
		//IL_000a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0010: Expected Obj, but got Unknown
		//IL_0036: Unknown result type (might be due to invalid IL or missing references)
		//IL_003c: Invalid comparison between Unknown and I4
		int try0001_dispatch = -1;
		int num2 = default;
		int num = default;
		while (true)
		{
			try
			{
				/*Note: ILSpy has introduced the following switch to emulate a goto from catch-block to try-block*/;
				switch (try0001_dispatch)
				{
				default:
				{
					ProjectData.ClearProjectError();
					num2 = 2;
					SaveFileDialog val = new SaveFileDialog();
					((FileDialog)val).FileName = only_excel_file_name;
					((FileDialog)val).DefaultExt = ".txt";
					((FileDialog)val).Filter = "XLSM Files (*.xlsm*)|*.xlsm";
					if ((int)((CommonDialog)val).ShowDialog() == 1)
					{
						string fileName = ((FileDialog)val).FileName;
						if (Operators.CompareString(Module1.source_Excel_file, "", TextCompare: false) != 0)
						{
							((ServerComputer)MyProject.Computer).FileSystem.CopyFile(Module1.source_Excel_file, fileName, UIOption.AllDialogs, UICancelOption.DoNothing);
							EXCEL_path = fileName;
							only_excel_file_name = Path.GetFileName(Module1.source_Excel_file);
						}
						MAIN_PROCESS(EXCEL_path);
						string value = Conversions.ToString((int)Interaction.MsgBox("File Successfully Created\r\n\r\n\r\nDo You Want to Open File ?", MsgBoxStyle.YesNo | MsgBoxStyle.Question, title_msg));
						if (Conversions.ToDouble(value) == 6.0)
						{
							Process.Start(EXCEL_path);
						}
						pb1.Value = 0;
						Label1.Text = "";
						((Control)pb1).Visible = false;
						((Control)Label1).Visible = false;
						break;
					}
					goto end_IL_0001;
				}
				case 368:
					num = -1;
					switch (num2)
					{
					case 2:
						break;
					default:
						goto end_IL_0001_2;
					}
					break;
				}
				if (Operators.CompareString(Information.Err().Description, "", TextCompare: false) != 0)
				{
					Interaction.MsgBox(Conversions.ToString(Information.Err().Number) + "\r\n" + Information.Err().Description);
				}
				break;
				end_IL_0001_2:;
			}
			catch (Exception ex) when ((num2 != 0) & (num == 0))
			{
				ProjectData.SetProjectError(ex);
				try0001_dispatch = 368;
				continue;
			}
			throw ProjectData.CreateProjectError(-2146828237);
			continue;
			end_IL_0001:
			break;
		}
		if (num != 0)
		{
			ProjectData.ClearProjectError();
		}
	}

	private void EXCEL_EDIT(string sheet_nm)
	{
		Application application = (Application)Activator.CreateInstance(Marshal.GetTypeFromCLSID(new Guid("00024500-0000-0000-C000-000000000046")));
		Workbook workbook = application.Workbooks.Open(EXCEL_path, RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
		Worksheet worksheet = (Worksheet)workbook.Worksheets[sheet_nm];
		application.DisplayAlerts = false;
		int num = 1;
		checked
		{
			do
			{
				string text = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[header_row, num], null, "value", new object[0], null, null, null));
				if (Operators.CompareString(text, "", TextCompare: false) != 0)
				{
					load_data1(text, sheet_nm);
					long num2 = starting_row;
					int num3 = variable1.Count - 1;
					for (int i = 0; i <= num3; i++)
					{
						if (Operators.CompareString(Strings.Trim(special_feild_name), Strings.Trim("Other Charges"), TextCompare: false) == 0)
						{
							worksheet.Cells[num2, 56] = variable1.Values.ElementAtOrDefault(i).ToString();
						}
						else if (Operators.CompareString(Strings.Trim(special_feild_name), Strings.Trim("Other charges"), TextCompare: false) == 0)
						{
							worksheet.Cells[num2, 68] = variable1.Values.ElementAtOrDefault(i).ToString();
						}
						else
						{
							worksheet.Cells[num2, num] = variable1.Values.ElementAtOrDefault(i).ToString();
						}
						num2++;
					}
				}
				num++;
			}
			while (num <= 80);
			workbook.Save();
			workbook.Close(RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
			application.DisplayAlerts = true;
			application.Quit();
			releaseObject(application);
			releaseObject(workbook);
			releaseObject(worksheet);
		}
	}

	private void MAIN_PROCESS(string field_nm)
	{
		//IL_0042: Unknown result type (might be due to invalid IL or missing references)
		//IL_0048: Expected Obj, but got Unknown
		pb1.Maximum = 100;
		((Control)pb1).Visible = true;
		((Control)Label1).Visible = true;
		con_access();
		string text = "SELECT DISTINCT (SheetName),Header_row,StartingRow FROM tempEinvoice";
		Application.DoEvents();
		OleDbDataAdapter val = new OleDbDataAdapter(text, Module1.con);
		DataSet dataSet = new DataSet();
		((DbDataAdapter)(object)val).Fill(dataSet);
		Application.DoEvents();
		pb1.Value = 10;
		Label1.Text = "10 % Completed";
		Thread.Sleep(100);
		checked
		{
			if (dataSet.Tables[0].Rows.Count > 0)
			{
				int num = dataSet.Tables[0].Rows.Count - 1;
				for (int i = 0; i <= num; i++)
				{
					header_row = Conversions.ToLong(dataSet.Tables[0].Rows[i][1].ToString());
					starting_row = (long)Math.Round(Conversion.Val(dataSet.Tables[0].Rows[i][2].ToString()));
					EXCEL_EDIT(dataSet.Tables[0].Rows[i][0].ToString());
				}
			}
			pb1.Value = 20;
			Label1.Text = "20 % Completed";
			Thread.Sleep(100);
			int num2 = 1;
			do
			{
				Application.DoEvents();
				pb1.Value = (int)Math.Round(Conversion.Val(pb1.Value) + 1.0);
				Label1.Text = Conversions.ToString(pb1.Value) + " % Completed";
				Thread.Sleep(10);
				num2++;
			}
			while (num2 <= 80);
		}
	}

	private void load_data1(string field_nm, string sheet_nm)
	{
		//IL_003a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0040: Expected Obj, but got Unknown
		con_access();
		string text = "select RowNo,Field_Value,Field_Name from tempEinvoice where Field_Name='" + field_nm + "' AND SheetName='" + sheet_nm + "'";
		OleDbDataAdapter val = new OleDbDataAdapter(text, Module1.con);
		DataSet dataSet = new DataSet();
		((DbDataAdapter)(object)val).Fill(dataSet);
		variable1.Clear();
		special_feild_name = field_nm;
		if (dataSet.Tables[0].Rows.Count <= 0)
		{
			return;
		}
		checked
		{
			int num = dataSet.Tables[0].Rows.Count - 1;
			for (int i = 0; i <= num; i++)
			{
				if (Operators.CompareString(dataSet.Tables[0].Rows[i][2].ToString(), Strings.Trim(field_nm), TextCompare: false) == 0)
				{
					variable1.Add(dataSet.Tables[0].Rows[i][0].ToString(), dataSet.Tables[0].Rows[i][1].ToString());
				}
			}
		}
	}

	private void Label1_Click(object sender, EventArgs e)
	{
	}

	private void Label1_Click_1(object sender, EventArgs e)
	{
	}

	private void releaseObject(object obj)
	{
		try
		{
			Marshal.ReleaseComObject(RuntimeHelpers.GetObjectValue(obj));
			obj = null;
		}
		catch (Exception ex)
		{
			ProjectData.SetProjectError(ex);
			Exception ex2 = ex;
			obj = null;
			ProjectData.ClearProjectError();
		}
		finally
		{
			GC.Collect();
		}
	}

	private void Button5_Click(object sender, EventArgs e)
	{
		Application application = (Application)Activator.CreateInstance(Marshal.GetTypeFromCLSID(new Guid("00024500-0000-0000-C000-000000000046")));
		Workbook workbook = application.Workbooks.Open(Module1.source_Excel_file, RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
		Worksheet worksheet = (Worksheet)workbook.Worksheets["Profile"];
		application.Visible = true;
	}

	private void Button6_Click(object sender, EventArgs e)
	{
		//IL_0001: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Expected Obj, but got Unknown
		//IL_002d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0033: Invalid comparison between Unknown and I4
		SaveFileDialog val = new SaveFileDialog();
		((FileDialog)val).FileName = only_excel_file_name;
		((FileDialog)val).DefaultExt = ".txt";
		((FileDialog)val).Filter = "XLSM Files (*.xlsm*)|*.xlsm";
		if ((int)((CommonDialog)val).ShowDialog() == 1)
		{
			string fileName = ((FileDialog)val).FileName;
			if (Operators.CompareString(Module1.source_Excel_file, "", TextCompare: false) != 0)
			{
				((ServerComputer)MyProject.Computer).FileSystem.CopyFile(Module1.source_Excel_file, fileName, UIOption.AllDialogs, UICancelOption.DoNothing);
				EXCEL_path = fileName;
				only_excel_file_name = Path.GetFileName(Module1.source_Excel_file);
			}
			string index = "Profile";
			Application application = (Application)Activator.CreateInstance(Marshal.GetTypeFromCLSID(new Guid("00024500-0000-0000-C000-000000000046")));
			Workbook workbook = application.Workbooks.Open(EXCEL_path, RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
			Worksheet worksheet = (Worksheet)workbook.Worksheets[index];
			application.DisplayAlerts = false;
			worksheet.Cells[3, 4] = "TEST";
			workbook.Save();
			workbook.Close(RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
			application.DisplayAlerts = true;
			application.Quit();
			releaseObject(application);
			releaseObject(workbook);
			releaseObject(worksheet);
		}
	}

	private void New_Json_file_create(string head_field)
	{
		//IL_0018: Unknown result type (might be due to invalid IL or missing references)
		//IL_001e: Expected Obj, but got Unknown
		string text = "SELECT FieldValue,FieldHeading_Jsn,FieldNm_Jsn,FieldType_Jsn from TempEInvoiceTable WHERE FieldHeading_Jsn<>'' AND FieldHeading_Jsn='" + head_field + "' and itemsrno=1";
		OleDbDataAdapter val = new OleDbDataAdapter(text, Module1.con);
		DataSet dataSet = new DataSet();
		((DbDataAdapter)(object)val).Fill(dataSet);
		DataTable dataTable = dataSet.Tables[0];
		if (dataSet.Tables[0].Rows.Count <= 0)
		{
			return;
		}
		checked
		{
			int num = dataTable.Rows.Count - 1;
			string text2 = default;
			for (int i = 0; i <= num; i++)
			{
				if (Operators.CompareString(dataTable.Rows[i]["FieldType_Jsn"].ToString(), "Text", TextCompare: false) == 0)
				{
					text2 = ((Operators.CompareString(dataTable.Rows[i]["FieldValue"].ToString(), "", TextCompare: false) != 0) ? ("\"" + dataTable.Rows[i]["FieldValue"].ToString() + "\"") : "null");
				}
				else if (Operators.CompareString(dataTable.Rows[i]["FieldType_Jsn"].ToString(), "Number", TextCompare: false) == 0)
				{
					text2 = ((Operators.CompareString(dataTable.Rows[i]["FieldValue"].ToString(), "", TextCompare: false) != 0) ? dataTable.Rows[i]["FieldValue"].ToString() : "0");
				}
				string text3 = dataTable.Rows[i]["FieldHeading_Jsn"].ToString();
				if (i == 0)
				{
					json_String = json_String + "\"" + text3 + "\":{";
					if (Operators.CompareString(head_field, "TranDtls", TextCompare: false) == 0)
					{
						json_String += "\"TaxSch\":\"GST\",";
					}
					json_String = json_String + "\"" + dataTable.Rows[i]["FieldNm_Jsn"].ToString() + "\":" + text2 + ",";
				}
				else if (i < dataTable.Rows.Count - 1)
				{
					json_String = json_String + "\"" + dataTable.Rows[i]["FieldNm_Jsn"].ToString() + "\":" + text2 + ",";
				}
				else
				{
					json_String = json_String + "\"" + dataTable.Rows[i]["FieldNm_Jsn"].ToString() + "\":" + text2;
				}
			}
			json_String += "},";
		}
	}

	private void Button7_Click(object sender, EventArgs e)
	{
		//IL_00c2: Unknown result type (might be due to invalid IL or missing references)
		//IL_00c9: Expected Obj, but got Unknown
		//IL_0183: Unknown result type (might be due to invalid IL or missing references)
		//IL_018a: Expected Obj, but got Unknown
		string text = "68c37f4121e93dc4b49b31820dd5602d7";
		string args = "{\"apikey\":\"" + text + "\",\"username\":\"" + Module1.username + "\",\"transaction_type\":\"" + Module1.transaction_type + "\",\"gstin\":\"" + Module1.gstin + "\",\"firm_name\":\"" + Module1.firm_name + "\",\"station\":\"" + Module1.station + "\"}";
		string url = "http://kamrasoftwares.com/epanel/web-service/index.php?service=addmanualprocess";
		upload_data(url, args);
		if (!Gen_JSON_File)
		{
			Interaction.MsgBox("Ehhh !!!\r\n\r\n\r\nApplication Error\r\n\r\n\r\nContact to Your Software Vendor\r\n", MsgBoxStyle.Critical, "Make JSON");
			return;
		}
		string text2 = "SELECT ItemSrNo, FieldValue,FieldHeading_Jsn,FieldNm_Jsn from TempEInvoiceTable WHERE FieldHeading_Jsn<>'' AND FieldHeading_Jsn='ItemList'";
		OleDbDataAdapter val = new OleDbDataAdapter(text2, Module1.con);
		DataSet dataSet = new DataSet();
		((DbDataAdapter)(object)val).Fill(dataSet);
		DataTable dataTable = dataSet.Tables[0];
		if (dataSet.Tables[0].Rows.Count == 0)
		{
			Interaction.MsgBox("Ehhh !!!\r\n\r\nNo Data Found For Make JSON File", MsgBoxStyle.Critical, "Make JSON File");
			return;
		}
		New_Json_file_create("TranDtls");
		New_Json_file_create("DocDtls");
		New_Json_file_create("SellerDtls");
		New_Json_file_create("BuyerDtls");
		New_Json_file_create("DispDtls");
		New_Json_file_create("ValDtls");
		New_Json_file_create("EwbDtls");
		text2 = "SELECT ItemSrNo, FieldValue,FieldHeading_Jsn,FieldNm_Jsn,FieldType_Jsn from TempEInvoiceTable WHERE FieldHeading_Jsn<>'' AND FieldHeading_Jsn='ItemList' order by srno";
		val = new OleDbDataAdapter(text2, Module1.con);
		dataSet = new DataSet();
		((DbDataAdapter)(object)val).Fill(dataSet);
		dataTable = dataSet.Tables[0];
		checked
		{
			if (dataSet.Tables[0].Rows.Count > 0)
			{
				int num = dataTable.Rows.Count - 1;
				string text3 = default;
				for (int i = 0; i <= num; i++)
				{
					if (Operators.CompareString(dataTable.Rows[i]["FieldType_Jsn"].ToString(), "Text", TextCompare: false) == 0)
					{
						text3 = ((Operators.CompareString(dataTable.Rows[i]["FieldValue"].ToString(), "", TextCompare: false) != 0) ? ("\"" + dataTable.Rows[i]["FieldValue"].ToString() + "\"") : "null");
					}
					else if (Operators.CompareString(dataTable.Rows[i]["FieldType_Jsn"].ToString(), "Number", TextCompare: false) == 0)
					{
						text3 = ((Operators.CompareString(dataTable.Rows[i]["FieldValue"].ToString(), "", TextCompare: false) != 0) ? dataTable.Rows[i]["FieldValue"].ToString() : "0");
					}
					string text4 = dataTable.Rows[i]["FieldHeading_Jsn"].ToString();
					if (i == 0)
					{
						json_String = json_String + "\"" + text4 + "\":[{";
						json_String = json_String + "\"" + dataTable.Rows[i]["FieldNm_Jsn"].ToString() + "\":" + text3 + ",";
					}
					else if (i < dataTable.Rows.Count - 1)
					{
						if (i < dataTable.Rows.Count)
						{
							if (Operators.CompareString(dataTable.Rows[i]["ItemSrNo"].ToString(), dataTable.Rows[i + 1]["ItemSrNo"].ToString(), TextCompare: false) != 0)
							{
								json_String = json_String + "\"" + dataTable.Rows[i]["FieldNm_Jsn"].ToString() + "\":" + text3 + ",";
								json_String = Strings.Mid(json_String, 1, json_String.Length - 1);
								json_String += "},{";
							}
							else
							{
								json_String = json_String + "\"" + dataTable.Rows[i]["FieldNm_Jsn"].ToString() + "\":" + text3 + ",";
							}
						}
					}
					else if (i > dataTable.Rows.Count)
					{
						if (Operators.CompareString(dataTable.Rows[i]["ItemSrNo"].ToString(), dataTable.Rows[i + 1]["ItemSrNo"].ToString(), TextCompare: false) == 0)
						{
							json_String = json_String + "\"" + dataTable.Rows[i]["FieldNm_Jsn"].ToString() + "\":" + text3;
							json_String = json_String + "\"" + text4 + "\":{";
						}
						else
						{
							json_String = json_String + "\"" + dataTable.Rows[i]["FieldNm_Jsn"].ToString() + "\":" + text3;
						}
					}
				}
				json_String += "}]";
				json_String = json_String.Replace(",}", "}");
				string text5 = "Version";
				string text6 = "1.1";
				json_String = "[{\"" + text5 + "\":\"" + text6 + "\"," + json_String + "}]";
				Write_Json_file(json_String);
			}
			else
			{
				Interaction.MsgBox("Ehhh !!!\r\n\r\nNo Data Found For Make JSON File", MsgBoxStyle.Critical, "Make JSON File");
			}
		}
	}

	private void upload_data(string url, string args)
	{
		Uri uri = new Uri(url);
		byte[] bytes = Encoding.UTF8.GetBytes(args);
		string text = SendRequest(uri, bytes, "application/json", "POST");
		JObject jObject = default;
		if (Operators.CompareString(text, "", TextCompare: false) != 0)
		{
			jObject = JObject.Parse(text);
		}
		if (Conversions.ToDouble(jObject["success"].ToString()) == 1.0)
		{
			Gen_JSON_File = true;
			return;
		}
		Gen_JSON_File = false;
		Interaction.MsgBox("Ehhh !!!\r\n\r\n\r\nApplication Error\r\n\r\n\r\nContact to Your Software Vendor\r\n\r\nJson Response : " + jObject["msg"].ToString(), MsgBoxStyle.Critical, "Make JSON");
	}

	public string SendRequest(Uri uri, byte[] jsonDataBytes, string contentType, string method)
	{
		ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
		WebRequest webRequest = WebRequest.Create(uri);
		webRequest.ContentType = contentType;
		webRequest.Method = method;
		webRequest.ContentLength = jsonDataBytes.Length;
		Stream requestStream = webRequest.GetRequestStream();
		requestStream.Write(jsonDataBytes, 0, jsonDataBytes.Length);
		requestStream.Close();
		Stream responseStream = webRequest.GetResponse().GetResponseStream();
		StreamReader streamReader = new StreamReader(responseStream);
		string result = streamReader.ReadToEnd();
		streamReader.Close();
		responseStream.Close();
		return result;
	}

	private void Write_Json_file(string data)
	{
		string text = Conversions.ToString(DateTime.Now);
		Module1.json_file_nm = Module1.json_file_nm.Replace(" ", "~");
		Module1.json_file_nm = Module1.json_file_nm.Replace(":", "-");
		string text2 = Application.StartupPath + "\\" + Module1.json_file_nm + ".Json";
		StreamWriter streamWriter = new StreamWriter(text2);
		streamWriter.Write(data);
		streamWriter.Close();
		MsgBoxResult msgBoxResult = Interaction.MsgBox("JSON File Successfully Generated\r\n\r\n\r\n" + text2 + "\r\n\r\n\r\nDo You Want to Open E-Invoice Portal\r\n", MsgBoxStyle.YesNo | MsgBoxStyle.Information, "Bahi-Khata E-Invoice System");
		if (msgBoxResult == MsgBoxResult.Yes)
		{
			Process.Start("https://einvoice1.gst.gov.in/");
		}
	}

	private void Form1_KeyDown(object sender, KeyEventArgs e)
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0009: Invalid comparison between Unknown and I4
		if ((int)e.KeyCode == 112)
		{
			if (((Control)Button2).Visible)
			{
				((Control)this).Height = 273;
				((Control)lblIRNno).Visible = false;
				((Control)Button2).Visible = false;
				((Control)PictureBox1).Visible = false;
			}
			else
			{
				((Control)this).Height = 511;
				((Control)lblIRNno).Visible = true;
				((Control)Button2).Visible = true;
				((Control)PictureBox1).Visible = true;
			}
		}
	}

	[DebuggerNonUserCode]
	protected override void Dispose(bool disposing)
	{
		try
		{
			if (disposing && components != null)
			{
				components.Dispose();
			}
		}
		finally
		{
			((Form)this).Dispose(disposing);
		}
	}

	[DebuggerStepThrough]
	private void InitializeComponent()
	{
		//IL_0012: Unknown result type (might be due to invalid IL or missing references)
		//IL_001c: Expected Obj, but got Unknown
		//IL_001e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0028: Expected Obj, but got Unknown
		//IL_002a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0034: Expected Obj, but got Unknown
		//IL_0036: Unknown result type (might be due to invalid IL or missing references)
		//IL_0040: Expected Obj, but got Unknown
		//IL_0042: Unknown result type (might be due to invalid IL or missing references)
		//IL_004c: Expected Obj, but got Unknown
		//IL_004e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0058: Expected Obj, but got Unknown
		//IL_005a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0064: Expected Obj, but got Unknown
		//IL_0066: Unknown result type (might be due to invalid IL or missing references)
		//IL_0070: Expected Obj, but got Unknown
		//IL_0072: Unknown result type (might be due to invalid IL or missing references)
		//IL_007c: Expected Obj, but got Unknown
		//IL_007e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0088: Expected Obj, but got Unknown
		//IL_008a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0094: Expected Obj, but got Unknown
		//IL_0096: Unknown result type (might be due to invalid IL or missing references)
		//IL_00a0: Expected Obj, but got Unknown
		//IL_00d6: Unknown result type (might be due to invalid IL or missing references)
		//IL_00e0: Expected Obj, but got Unknown
		//IL_016f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0179: Expected Obj, but got Unknown
		//IL_0313: Unknown result type (might be due to invalid IL or missing references)
		//IL_031d: Expected Obj, but got Unknown
		//IL_039a: Unknown result type (might be due to invalid IL or missing references)
		//IL_03a4: Expected Obj, but got Unknown
		//IL_048d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0497: Expected Obj, but got Unknown
		//IL_0546: Unknown result type (might be due to invalid IL or missing references)
		//IL_0550: Expected Obj, but got Unknown
		//IL_065a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0664: Expected Obj, but got Unknown
		//IL_0808: Unknown result type (might be due to invalid IL or missing references)
		//IL_0812: Expected Obj, but got Unknown
		ComponentResourceManager componentResourceManager = new ComponentResourceManager(typeof(Form1));
		Button1 = new Button();
		Button2 = new Button();
		Button3 = new Button();
		PictureBox1 = new PictureBox();
		Button4 = new Button();
		lblIRNno = new Label();
		SaveFileDialog1 = new SaveFileDialog();
		pb1 = new ProgressBar();
		Label1 = new Label();
		Button5 = new Button();
		Button6 = new Button();
		Button7 = new Button();
		((ISupportInitialize)PictureBox1).BeginInit();
		((Control)this).SuspendLayout();
		((ButtonBase)Button1).BackColor = Color.White;
		((Control)Button1).Font = new Font("Verdana", 14f, (FontStyle)1);
		((Control)Button1).Location = new Point(235, 41);
		((Control)Button1).Name = "Button1";
		((Control)Button1).Size = new Size(242, 112);
		((Control)Button1).TabIndex = 0;
		((ButtonBase)Button1).Text = "Open E-Invoice Govt. Portal";
		((ButtonBase)Button1).UseVisualStyleBackColor = false;
		((ButtonBase)Button2).BackColor = Color.White;
		((Control)Button2).Font = new Font("Verdana", 14f, (FontStyle)1);
		((Control)Button2).Location = new Point(387, 277);
		((Control)Button2).Name = "Button2";
		((Control)Button2).Size = new Size(191, 112);
		((Control)Button2).TabIndex = 1;
		((ButtonBase)Button2).Text = "Get QR Code && IRN No. From E-Invoice Website";
		((ButtonBase)Button2).UseVisualStyleBackColor = false;
		((Control)Button2).Visible = false;
		((Control)Button3).Location = new Point(617, 161);
		((Control)Button3).Name = "Button3";
		((Control)Button3).Size = new Size(101, 32);
		((Control)Button3).TabIndex = 2;
		((ButtonBase)Button3).Text = "Button3";
		((ButtonBase)Button3).UseVisualStyleBackColor = true;
		((Control)Button3).Visible = false;
		PictureBox1.BorderStyle = (BorderStyle)1;
		((Control)PictureBox1).Location = new Point(610, 277);
		((Control)PictureBox1).Name = "PictureBox1";
		((Control)PictureBox1).Size = new Size(123, 112);
		PictureBox1.SizeMode = (PictureBoxSizeMode)1;
		PictureBox1.TabIndex = 3;
		PictureBox1.TabStop = false;
		((Control)PictureBox1).Visible = false;
		((ButtonBase)Button4).BackColor = Color.White;
		((Control)Button4).Font = new Font("Verdana", 14f, (FontStyle)1);
		((Control)Button4).Location = new Point(12, 41);
		((Control)Button4).Name = "Button4";
		((Control)Button4).Size = new Size(217, 112);
		((Control)Button4).TabIndex = 0;
		((ButtonBase)Button4).Text = "Send Data to Govt. Excel Template";
		((ButtonBase)Button4).UseVisualStyleBackColor = false;
		((Control)lblIRNno).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
		((Control)lblIRNno).Location = new Point(84, 408);
		((Control)lblIRNno).Name = "lblIRNno";
		((Control)lblIRNno).Size = new Size(649, 47);
		((Control)lblIRNno).TabIndex = 5;
		lblIRNno.Text = "Label1";
		((Control)lblIRNno).Visible = false;
		((Control)pb1).Location = new Point(12, 200);
		((Control)pb1).Name = "pb1";
		((Control)pb1).Size = new Size(675, 24);
		pb1.Style = (ProgressBarStyle)1;
		((Control)pb1).TabIndex = 15;
		((Control)pb1).Visible = false;
		((Control)Label1).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
		((Control)Label1).ForeColor = Color.Red;
		((Control)Label1).Location = new Point(9, 180);
		((Control)Label1).Name = "Label1";
		((Control)Label1).Size = new Size(334, 17);
		((Control)Label1).TabIndex = 14;
		Label1.Text = "Label1";
		Label1.TextAlign = (ContentAlignment)16;
		Label1.UseMnemonic = false;
		((ButtonBase)Button5).BackColor = Color.White;
		((Control)Button5).Font = new Font("Verdana", 14f, (FontStyle)1);
		((Control)Button5).Location = new Point(483, 43);
		((Control)Button5).Name = "Button5";
		((Control)Button5).Size = new Size(235, 112);
		((Control)Button5).TabIndex = 16;
		((ButtonBase)Button5).Text = "Set Your Profile in Excel Format";
		((ButtonBase)Button5).UseVisualStyleBackColor = false;
		((Control)Button6).Location = new Point(497, 159);
		((Control)Button6).Name = "Button6";
		((Control)Button6).Size = new Size(114, 37);
		((Control)Button6).TabIndex = 17;
		((ButtonBase)Button6).Text = "Button6";
		((ButtonBase)Button6).UseVisualStyleBackColor = true;
		((Control)Button6).Visible = false;
		((ButtonBase)Button7).BackColor = Color.White;
		((Control)Button7).Font = new Font("Verdana", 14f, (FontStyle)1);
		((Control)Button7).Location = new Point(12, 2);
		((Control)Button7).Name = "Button7";
		((Control)Button7).Size = new Size(217, 33);
		((Control)Button7).TabIndex = 18;
		((ButtonBase)Button7).Text = "Make JSON File";
		((ButtonBase)Button7).UseVisualStyleBackColor = false;
		((Control)Button7).Visible = false;
		((ContainerControl)this).AutoScaleDimensions = new SizeF(6f, 13f);
		((ContainerControl)this).AutoScaleMode = (AutoScaleMode)1;
		((Form)this).BackColor = Color.FromArgb(255, 255, 192);
		((Form)this).ClientSize = new Size(724, 234);
		((Control)this).Controls.Add((Control)(object)Button7);
		((Control)this).Controls.Add((Control)(object)Button6);
		((Control)this).Controls.Add((Control)(object)Button5);
		((Control)this).Controls.Add((Control)(object)pb1);
		((Control)this).Controls.Add((Control)(object)Label1);
		((Control)this).Controls.Add((Control)(object)lblIRNno);
		((Control)this).Controls.Add((Control)(object)Button4);
		((Control)this).Controls.Add((Control)(object)PictureBox1);
		((Control)this).Controls.Add((Control)(object)Button3);
		((Control)this).Controls.Add((Control)(object)Button2);
		((Control)this).Controls.Add((Control)(object)Button1);
		((Control)this).ForeColor = Color.Black;
		((Form)this).Icon = (Icon)componentResourceManager.GetObject("$this.Icon");
		((Form)this).MaximizeBox = false;
		((Form)this).MinimizeBox = false;
		((Control)this).Name = "Form1";
		((Form)this).StartPosition = (FormStartPosition)1;
		((Form)this).Text = "Form1";
		((ISupportInitialize)PictureBox1).EndInit();
		((Control)this).ResumeLayout(false);
	}
}
