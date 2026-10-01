using System;
using System.Windows.Forms;
using MatchGSTR2.My;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.CompilerServices;

namespace MatchGSTR2;

[StandardModule]
internal sealed class Module1
{
	public static string save_file_nm;

	[STAThread]
	public static void Main(string[] args)
	{
		//IL_00f7: Unknown result type (might be due to invalid IL or missing references)
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
		}
		else
		{
			save_file_nm = Strings.Mid(args[0].ToString(), checked(Conversion.Val(inputStr).ToString().Length + 1), args[0].Length);
			save_file_nm = save_file_nm.Replace("^", " ");
			((Form)MyProject.Forms.Form1).ShowDialog();
		}
	}
}
