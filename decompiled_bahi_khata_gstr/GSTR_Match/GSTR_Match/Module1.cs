using System;
using System.Data;
using System.Data.Common;
using System.Data.OleDb;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using GSTR_Match.My;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.CompilerServices;
using Microsoft.VisualBasic.Devices;

namespace GSTR_Match;

[StandardModule]
internal sealed class Module1
{
	public static OleDbConnection con = new OleDbConnection();

	public static string auth_Token;

	public static bool local_run = false;

	public static string db_path = "";

	public static string frmperiod;

	public static string toperiod;

	public static string GSTIN1;

	public static string PortalUserName;

	public static string Bahi_khata_ret_period;

	public static string period_name;

	public static bool OTP_exit;

	public static string firm_name;

	private static bool field_exists;

	static Module1()
	{
		//IL_0000: Unknown result type (might be due to invalid IL or missing references)
		//IL_000a: Expected Obj, but got Unknown
	}

	[STAThread]
	public static void Main(string[] args)
	{
		//IL_0061: Unknown result type (might be due to invalid IL or missing references)
		//IL_0202: Unknown result type (might be due to invalid IL or missing references)
		string text = Application.StartupPath + "\\Local.txt";
		if (((ServerComputer)MyProject.Computer).FileSystem.FileExists(text))
		{
			local_run = true;
		}
		else
		{
			local_run = false;
		}
		if (local_run)
		{
			frmperiod = "01-Jan-2023";
			toperiod = "31-Jan-2023";
			((Form)MyProject.Forms.Form1).ShowDialog();
			return;
		}
		if (args.Length <= 0)
		{
			Interaction.MsgBox("Invalid Licence ID Found", MsgBoxStyle.Critical, "Kamra Softwares");
			Application.Exit();
			return;
		}
		string inputStr = Strings.Format(DateAndTime.Today.Date, "dd").ToString();
		string inputStr2 = Strings.Left(args[0].ToString(), Conversion.Val(inputStr).ToString().Length);
		if (Conversion.Val(inputStr2) != Conversion.Val(inputStr))
		{
			Interaction.MsgBox("Invalid Licence ID Found", MsgBoxStyle.Critical, "Kamra Softwares");
			Application.Exit();
			return;
		}
		checked
		{
			string left = Strings.Mid(args[0].ToString(), Conversion.Val(inputStr).ToString().Length + 1, 5);
			if (Conversion.Val(args.Count().ToString()) >= 0.0)
			{
				if (Operators.CompareString(left, "Path=", TextCompare: false) == 0)
				{
					db_path = Strings.Mid(args[0].ToString(), Conversion.Val(inputStr).ToString().Length + 1, args[0].Length);
					db_path = db_path.Replace("Path=", "");
					db_path = db_path.Replace("^", " ");
					frmperiod = args[1].ToString();
					toperiod = args[2].ToString();
					((Form)MyProject.Forms.Form1).ShowDialog();
				}
				else
				{
					Interaction.MsgBox("Invalid Licence ID Found", MsgBoxStyle.Critical, "Kamra Softwares");
					Application.Exit();
				}
			}
		}
	}

	public static void CONN()
	{
		if (con.State == ConnectionState.Open)
		{
			con.Close();
		}
		string text = ((Operators.CompareString(db_path, "", TextCompare: false) != 0) ? db_path : (Application.StartupPath + "\\Data.008"));
		con.ConnectionString = "Provider=Microsoft.ACE.OLEDB.12.0;Data Source=" + text + ";Persist Security Info=False";
		con.Open();
	}

	public static string get_single_value(string get_field_nm, string tbl_nm, string check_field, string val1)
	{
		//IL_0001: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Expected Obj, but got Unknown
		//IL_005e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0064: Expected Obj, but got Unknown
		OleDbDataAdapter val2 = new OleDbDataAdapter();
		DataSet dataSet = new DataSet();
		CONN();
		string text = "select " + get_field_nm + " from " + tbl_nm + " where " + check_field + "='" + val1 + "'";
		val2 = new OleDbDataAdapter(text, con);
		dataSet = new DataSet();
		((DbDataAdapter)(object)val2).Fill(dataSet);
		if (dataSet.Tables[0].Rows.Count > 0)
		{
			return dataSet.Tables[0].Rows[0].ItemArray[0].ToString();
		}
		string result = default;
		return result;
	}

	public static string get_single_value_NO_ARGS(string get_field_nm, string tbl_nm)
	{
		//IL_000a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0010: Expected Obj, but got Unknown
		//IL_0040: Unknown result type (might be due to invalid IL or missing references)
		//IL_0046: Expected Obj, but got Unknown
		int try0001_dispatch = -1;
		int num2 = default;
		string text2 = default;
		string result;
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
					OleDbDataAdapter val = new OleDbDataAdapter();
					DataSet dataSet = new DataSet();
					CONN();
					string text = ("select " + get_field_nm + " from " + tbl_nm) ?? "";
					val = new OleDbDataAdapter(text, con);
					dataSet = new DataSet();
					((DbDataAdapter)(object)val).Fill(dataSet);
					if (dataSet.Tables[0].Rows.Count > 0)
					{
						text2 = dataSet.Tables[0].Rows[0].ItemArray[0].ToString();
					}
					result = text2;
					goto end_IL_0001;
				}
				case 174:
					num = -1;
					switch (num2)
					{
					case 2:
						result = "";
						goto end_IL_0001;
					}
					break;
				}
			}
			catch (Exception ex) when ((num2 != 0) & (num == 0))
			{
				ProjectData.SetProjectError(ex);
				try0001_dispatch = 174;
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
		return result;
	}

	public static void CREATE_NEW_TABLE(string tbl_nm, string qry, OleDbConnection c)
	{
		//IL_003d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0043: Expected Obj, but got Unknown
		DataTable oleDbSchemaTable = con.GetOleDbSchemaTable(OleDbSchemaGuid.Tables, new object[4] { null, null, tbl_nm, "TABLE" });
		if (oleDbSchemaTable.Rows.Count <= 0)
		{
			OleDbCommand val = new OleDbCommand(qry, c);
			val.ExecuteNonQuery();
		}
	}

	public static void ADD_Field(string tbl_nm, string field_nm, string data_type, OleDbConnection c, string defval)
	{
		//IL_0015: Unknown result type (might be due to invalid IL or missing references)
		//IL_001b: Expected Obj, but got Unknown
		//IL_0129: Unknown result type (might be due to invalid IL or missing references)
		//IL_0130: Expected Obj, but got Unknown
		field_exists = false;
		string text = "Select * from " + tbl_nm;
		OleDbDataAdapter val = new OleDbDataAdapter(text, c);
		DataSet dataSet = new DataSet();
		((DbDataAdapter)(object)val).Fill(dataSet);
		DataTable dataTable = dataSet.Tables[0];
		checked
		{
			int num = dataTable.Columns.Count - 1;
			for (int i = 0; i <= num; i++)
			{
				if (Operators.CompareString(Strings.UCase(dataTable.Columns[i].ColumnName), Strings.UCase(field_nm), TextCompare: false) == 0)
				{
					field_exists = true;
					break;
				}
			}
			if (!field_exists)
			{
				text = ((Operators.CompareString(defval, "Yes", TextCompare: false) != 0) ? ("ALTER TABLE " + tbl_nm + " ADD COLUMN " + field_nm + " " + data_type + " ") : ("ALTER TABLE " + tbl_nm + " ADD COLUMN " + field_nm + " " + data_type + " DEFAULT 0"));
				OleDbCommand val2 = new OleDbCommand(text, c);
				val2.ExecuteNonQuery();
			}
		}
	}

	[DllImport("user32.dll", CharSet = CharSet.Unicode, EntryPoint = "LoadCursorFromFileW", ExactSpelling = true, SetLastError = true)]
	private static extern IntPtr LoadCursorFromFile([MarshalAs(UnmanagedType.VBByRefStr)] ref string filename);

	public static Cursor CreateCursor(string filename)
	{
		//IL_002e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0034: Expected Obj, but got Unknown
		Cursor result = null;
		try
		{
			IntPtr intPtr = LoadCursorFromFile(ref filename);
			if (IntPtr.Zero.Equals((object?)(nint)intPtr))
			{
				throw new ApplicationException("Could not create cursor from file ");
			}
			result = new Cursor(intPtr);
		}
		catch (Exception ex)
		{
			ProjectData.SetProjectError(ex);
			Exception ex2 = ex;
			ProjectData.ClearProjectError();
		}
		return result;
	}
}
