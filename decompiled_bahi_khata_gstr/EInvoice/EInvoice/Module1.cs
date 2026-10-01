using System;
using System.Data.OleDb;
using System.Linq;
using System.Windows.Forms;
using EInvoice.My;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.CompilerServices;
using Microsoft.VisualBasic.Devices;

namespace EInvoice;

[StandardModule]
internal sealed class Module1
{
	public static string source_Excel_file;

	public static string target_Excel_file;

	public static string db_path;

	public static bool with_args;

	private static bool local = false;

	public static string username;

	public static string transaction_type;

	public static string gstin;

	public static string firm_name;

	public static string station;

	public static string FormOpen;

	public static string json_file_nm;

	public static OleDbConnection con = new OleDbConnection();

	static Module1()
	{
		//IL_0006: Unknown result type (might be due to invalid IL or missing references)
		//IL_0010: Expected Obj, but got Unknown
	}

	[STAThread]
	public static void Main(string[] args)
	{
		//IL_009b: Unknown result type (might be due to invalid IL or missing references)
		//IL_02dd: Unknown result type (might be due to invalid IL or missing references)
		if (((ServerComputer)MyProject.Computer).FileSystem.FileExists(Application.StartupPath + "\\Local.txt"))
		{
			local = true;
		}
		if (local)
		{
			username = "Techbeat Solutions Rajkot";
			transaction_type = "EInvoice";
			gstin = "06ASDFJ1525L1ZZ";
			station = "BBD CBD";
			firm_name = "ABCD Trading Co";
			FormOpen = Conversions.ToString(0);
			json_file_nm = "05-08-2023 8:21:54 PM";
			db_path = Application.StartupPath + "\\data.001";
			((Form)MyProject.Forms.Form1).ShowDialog();
			return;
		}
		if (args.Length <= 0)
		{
			Interaction.MsgBox("Invalid Licence ID Found", MsgBoxStyle.Critical, "Kamra Softwares");
			Application.Exit();
			return;
		}
		checked
		{
			int num = args.Count() - 1;
			string text = default;
			for (int i = 0; i <= num; i++)
			{
				text = text + " " + args[i];
			}
			string inputStr = Strings.Format(DateAndTime.Today.Date, "dd").ToString();
			string inputStr2 = Strings.Left(args[0].ToString(), Conversion.Val(inputStr).ToString().Length);
			if (Conversion.Val(inputStr2) != Conversion.Val(inputStr))
			{
				Interaction.MsgBox("Invalid Licence ID Found", MsgBoxStyle.Critical, "Kamra Softwares");
				Application.Exit();
				return;
			}
			source_Excel_file = args[1];
			db_path = args[2];
			if (Conversion.Val(args.Count().ToString()) > 3.0)
			{
				username = args[3];
				transaction_type = args[4];
				gstin = args[5];
				firm_name = args[6];
				station = args[7];
				FormOpen = args[8];
				json_file_nm = args[9];
				username = username.Replace("^", " ");
				transaction_type = transaction_type.Replace("^", " ");
				gstin = gstin.Replace("^", " ");
				station = station.Replace("^", " ");
				firm_name = firm_name.Replace("^", " ");
				FormOpen = FormOpen.Replace("^", " ");
				json_file_nm = json_file_nm.Replace("^", " ");
			}
			source_Excel_file = source_Excel_file.Replace("^", " ");
			db_path = db_path.Replace("^", " ");
			with_args = true;
			((Form)MyProject.Forms.Form1).ShowDialog();
		}
	}
}
