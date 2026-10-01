using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Data.Common;
using System.Data.OleDb;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Net;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Aspose.Cells;
using Aspose.Cells.Utility;
using GSTR_Match.My;
using Microsoft.Office.Interop.Excel;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.CompilerServices;
using Microsoft.VisualBasic.Devices;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using ThirdParty.Json.LitJson;

namespace GSTR_Match;

[DesignerGenerated]
public class Form1 : Form
{
	private OleDbDataAdapter da;

	private DataSet ds;

	private DataTable dt;

	private DateTime otp_exp_date;

	private string period1;

	private string file_nm;

	private string cursor_file;

	private bool stop_process;

	private string Stored_OTP;

	private double match;

	private double mismatch;

	private double notinportal;

	private double notinbahikhata;

	private double tot_match_Amt;

	private double tot_match_taxable;

	private double tot_match_IGST;

	private double tot_match_CGST;

	private double tot_match_SGST;

	private double tot_match_CESS;

	private double tot_mismatch_Amt;

	private double tot_mismatch_taxable;

	private double tot_mismatch_IGST;

	private double tot_mismatch_CGST;

	private double tot_mismatch_SGST;

	private double tot_mismatch_CESS;

	private double tot_notinportal_Amt;

	private double tot_notinportal_taxable;

	private double tot_notinportal_IGST;

	private double tot_notinportal_CGST;

	private double tot_notinportal_SGST;

	private double tot_notinportal_CESS;

	private double tot_notinbahikhata_Amt;

	private double tot_notinbahikhata_taxable;

	private double tot_notinbahikhata_IGST;

	private double tot_notinbahikhata_CGST;

	private double tot_notinbahikhata_SGST;

	private double tot_notinbahikhata_CESS;

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

	[field: AccessedThroughProperty("BTN_GEN_Token")]
	internal virtual Button BTN_GEN_Token
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = BTN_GEN_Token_Click;
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

	[field: AccessedThroughProperty("BtnDownloadData")]
	internal virtual Button BtnDownloadData
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = BtnDownloadData_Click;
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

	[field: AccessedThroughProperty("TextBox1")]
	internal virtual TextBox TextBox1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("btnJsonToExcel")]
	internal virtual Button btnJsonToExcel
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = btnJsonToExcel_Click;
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

	[field: AccessedThroughProperty("btnExcelToAccess")]
	internal virtual Button btnExcelToAccess
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = btnExcelToAccess_Click;
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

	[field: AccessedThroughProperty("PB1")]
	internal virtual ProgressBar PB1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("DGVdata")]
	internal virtual DataGridView DGVdata
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("CHKDownloadAlways")]
	internal virtual CheckBox CHKDownloadAlways
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("RBAuto")]
	internal virtual RadioButton RBAuto
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = RBAuto_CheckedChanged;
			RadioButton val = field;
			if (val != null)
			{
				val.CheckedChanged -= eventHandler;
			}
			field = value;
			val = field;
			if (val != null)
			{
				val.CheckedChanged += eventHandler;
			}
		}
	}

	[field: AccessedThroughProperty("RBManual")]
	internal virtual RadioButton RBManual
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = RBManual_CheckedChanged;
			RadioButton val = field;
			if (val != null)
			{
				val.CheckedChanged -= eventHandler;
			}
			field = value;
			val = field;
			if (val != null)
			{
				val.CheckedChanged += eventHandler;
			}
		}
	}

	[field: AccessedThroughProperty("GroupBox1")]
	internal virtual GroupBox GroupBox1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("BtnGetOTP")]
	internal virtual Button BtnGetOTP
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = BtnGetOTP_Click;
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

	[field: AccessedThroughProperty("BtnVerifyOTP")]
	internal virtual Button BtnVerifyOTP
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = BtnVerifyOTP_Click;
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

	[field: AccessedThroughProperty("LBL_Progress")]
	internal virtual Label LBL_Progress
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("txtPath")]
	internal virtual TextBox txtPath
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
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

	[field: AccessedThroughProperty("OpenFileDialog1")]
	internal virtual OpenFileDialog OpenFileDialog1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
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

	[field: AccessedThroughProperty("BTNCompare")]
	internal virtual Button BTNCompare
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = BTNCompare_Click;
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

	[field: AccessedThroughProperty("DGVBahikhata_data")]
	internal virtual DataGridView DGVBahikhata_data
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("DGVPortal_Data")]
	internal virtual DataGridView DGVPortal_Data
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("txtBillno")]
	internal virtual TextBox txtBillno
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
		set;
	}

	[field: AccessedThroughProperty("Label2")]
	internal virtual Label Label2
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("txtAmount")]
	internal virtual TextBox txtAmount
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("DGVFinal")]
	internal virtual DataGridView DGVFinal
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("TabControl1")]
	internal virtual TabControl TabControl1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("TabPage1")]
	internal virtual TabPage TabPage1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("TabPage2")]
	internal virtual TabPage TabPage2
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("TabPage3")]
	internal virtual TabPage TabPage3
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("btnExcel")]
	internal virtual Button btnExcel
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = btnExcel_Click;
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

	[field: AccessedThroughProperty("SaveFileDialog1")]
	internal virtual SaveFileDialog SaveFileDialog1
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

	[field: AccessedThroughProperty("PictureBox1")]
	internal virtual PictureBox PictureBox1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
	}

	[field: AccessedThroughProperty("LblPeriod")]
	internal virtual Label LblPeriod
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
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

	[field: AccessedThroughProperty("FolderBrowserDialog1")]
	internal virtual FolderBrowserDialog FolderBrowserDialog1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
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

	public Form1()
	{
		//IL_0020: Unknown result type (might be due to invalid IL or missing references)
		//IL_002a: Expected Obj, but got Unknown
		((Form)this).Load += Form1_Load;
		((Control)this).KeyDown += Form1_KeyDown;
		InitializeComponent();
	}

	private void Form1_Load(object sender, EventArgs e)
	{
		//IL_01eb: Unknown result type (might be due to invalid IL or missing references)
		//IL_01f1: Expected Obj, but got Unknown
		//IL_020c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0212: Expected Obj, but got Unknown
		//IL_02d5: Unknown result type (might be due to invalid IL or missing references)
		//IL_02df: Expected Obj, but got Unknown
		//IL_0250: Unknown result type (might be due to invalid IL or missing references)
		//IL_0256: Expected Obj, but got Unknown
		//IL_0519: Unknown result type (might be due to invalid IL or missing references)
		//IL_0520: Expected Obj, but got Unknown
		((Form)this).KeyPreview = true;
		((Control)DGVPortal_Data).Visible = false;
		((Control)DGVBahikhata_data).Visible = false;
		LBL_Progress.Text = "";
		Module1.CONN();
		DGVdata.RowCount = 1;
		DGVdata.ColumnCount = 20;
		stop_process = false;
		if (Module1.local_run)
		{
			period1 = "&ret_period=082022";
		}
		LblPeriod.Text = "Date Period From : " + Module1.frmperiod + " To " + Module1.toperiod;
		string qry = "CREATE TABLE OtherSettings (LabourRt Text);";
		Module1.CREATE_NEW_TABLE("OtherSettings", qry, Module1.con);
		Module1.ADD_Field("OtherSettings", "LabourRt", "Text", Module1.con, "No");
		Module1.ADD_Field("OtherSettings", "tmpdttm", "datetime", Module1.con, "No");
		Module1.ADD_Field("OtherSettings", "Busiregngstr2", "int", Module1.con, "Yes");
		Module1.ADD_Field("OtherSettings", "LabourRtVry", "int", Module1.con, "Yes");
		qry = "CREATE TABLE TmpTbl (gst Text, party Text, ret_date Text, period Text, inv_no Text, types Text, inv_dt date, amount Text, pos Text, rev Text, itcavl Text, rsn Text, diffprcnt Text, num Text, rt Text, txval Text, igst Text, cess Text, cgst Text, sgst Text, type1 Text, period1 Text);";
		Module1.CREATE_NEW_TABLE("TmpTbl", qry, Module1.con);
		Module1.firm_name = Module1.get_single_value_NO_ARGS("Companyname", "CompanyInfo");
		string text = Module1.get_single_value_NO_ARGS("gstin", "CompanyInfo");
		file_nm = "GSTR-2 Match " + text + " " + Module1.firm_name + " " + Module1.frmperiod + " To " + Module1.toperiod;
		((Form)this).Text = ((Form)this).Text + "   " + Module1.firm_name + "   " + text;
		OleDbDataAdapter val = new OleDbDataAdapter();
		DataSet dataSet = new DataSet();
		Module1.CONN();
		string text2 = "Select Top 1 * From SaleTransportationDetail Where Len(IRNNo)>0";
		val = new OleDbDataAdapter(text2, Module1.con);
		dataSet = new DataSet();
		((DbDataAdapter)(object)val).Fill(dataSet);
		if (dataSet.Tables[0].Rows.Count == 0)
		{
			text2 = "Select Top 1 * From CottonVouchers Where Len(IRNNo)>0";
			val = new OleDbDataAdapter(text2, Module1.con);
			dataSet = new DataSet();
			((DbDataAdapter)(object)val).Fill(dataSet);
			if (dataSet.Tables[0].Rows.Count == 0)
			{
				RBManual.Checked = true;
				((Control)RBAuto).Visible = false;
				((Control)RBManual).Visible = false;
			}
		}
		cursor_file = Application.StartupPath + "\\wait.ico";
		string text3 = "SELECT * from OtherSettings";
		da = new OleDbDataAdapter(text3, Module1.con);
		ds = new DataSet();
		((DbDataAdapter)(object)da).Fill(ds);
		dt = ds.Tables[0];
		if (dt.Rows.Count > 0)
		{
			Stored_OTP = dt.Rows[0]["LabourRt"].ToString();
			if (Operators.CompareString(dt.Rows[0]["tmpdttm"].ToString(), "", TextCompare: false) != 0)
			{
				otp_exp_date = Conversions.ToDate(dt.Rows[0]["tmpdttm"].ToString());
			}
		}
		Module1.GSTIN1 = Module1.get_single_value_NO_ARGS("GSTIN", "CompanyInfo");
		Module1.PortalUserName = Module1.get_single_value_NO_ARGS("GSTPortalUserName", "VoucherSettings");
		Module1.auth_Token = "ef6599bdb1b1d091c56a5fbef6fab0af4b38f692";
		string left = Module1.get_single_value_NO_ARGS("Busiregngstr2", "OtherSettings");
		if (Operators.CompareString(left, "1", TextCompare: false) != 0)
		{
			string auth_Token = Module1.auth_Token;
			string gSTIN = Module1.GSTIN1;
			string portalUserName = Module1.PortalUserName;
			string s = "{\"access_token\":\"" + auth_Token + "\",\"gstin_number\":\"" + gSTIN + "\",\"gst_userName\":\"" + Module1.PortalUserName + "\"}";
			string uriString = "https://pro.mastersindia.co/bussiness/checkGstin";
			Uri uri = new Uri(uriString);
			byte[] bytes = Encoding.UTF8.GetBytes(s);
			string text4 = SendRequest(uri, bytes, "application/json", "POST");
			if (Operators.CompareString(text4, "", TextCompare: false) == 0)
			{
				Interaction.MsgBox("Business Registration Failed");
			}
			else
			{
				string text5 = JsonConvert.SerializeObject(text4);
				JObject jObject = JObject.Parse(text4);
				string left2 = jObject["status"].ToString().Replace("\"", "");
				if (Operators.CompareString(left2, "Success", TextCompare: false) == 0)
				{
					string text6 = "Update OtherSettings set Busiregngstr2='1'";
					OleDbCommand val2 = new OleDbCommand(text6, Module1.con);
					val2.ExecuteNonQuery();
				}
				else
				{
					Interaction.MsgBox("Business Registration Failed");
				}
			}
		}
		int height = ((Computer)MyProject.Computer).Screen.Bounds.Size.Height;
		int width = ((Computer)MyProject.Computer).Screen.Bounds.Size.Width;
		checked
		{
			((Control)TabControl1).Height = height - 245;
			((Control)TabControl1).Width = width - 1;
			((Control)DGVFinal).Height = ((Control)TabControl1).Height;
			((Control)DGVFinal).Width = ((Control)TabControl1).Width;
		}
	}

	private void Show_Process_Label(string msg, string delay)
	{
		Application.DoEvents();
		LBL_Progress.Text = msg;
		Thread.Sleep(checked((int)Math.Round(Conversion.Val(delay))));
		Application.DoEvents();
	}

	private void Whole_PROCESS(object sender, EventArgs e)
	{
		//IL_001c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0026: Expected Obj, but got Unknown
		//IL_01a4: Unknown result type (might be due to invalid IL or missing references)
		//IL_01dd: Unknown result type (might be due to invalid IL or missing references)
		//IL_0181: Unknown result type (might be due to invalid IL or missing references)
		//IL_015f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0222: Unknown result type (might be due to invalid IL or missing references)
		//IL_039d: Unknown result type (might be due to invalid IL or missing references)
		//IL_03a7: Expected Obj, but got Unknown
		Module1.OTP_exit = false;
		Thread.Sleep(100);
		string text = "SELECT * from OtherSettings";
		da = new OleDbDataAdapter(text, Module1.con);
		ds = new DataSet();
		((DbDataAdapter)(object)da).Fill(ds);
		dt = ds.Tables[0];
		if (dt.Rows.Count > 0)
		{
			Stored_OTP = dt.Rows[0]["LabourRt"].ToString();
			if (Operators.CompareString(dt.Rows[0]["tmpdttm"].ToString(), "", TextCompare: false) != 0)
			{
				otp_exp_date = Conversions.ToDate(dt.Rows[0]["tmpdttm"].ToString());
				otp_exp_date = Conversions.ToDate(otp_exp_date.ToString("dd-MM-yyyy"));
			}
			DateTime now = DateTime.Now;
			int num = DateTime.Compare(now, otp_exp_date);
			if (num >= 0)
			{
				if (num == 0)
				{
					if (!Module1.OTP_exit)
					{
						((Form)MyProject.Forms.GET_and_VERIFY_OTP).ShowDialog();
					}
				}
				else if (!Module1.OTP_exit)
				{
					((Form)MyProject.Forms.GET_and_VERIFY_OTP).ShowDialog();
				}
			}
		}
		else if (!Module1.OTP_exit)
		{
			((Form)MyProject.Forms.GET_and_VERIFY_OTP).ShowDialog();
		}
		if (Operators.CompareString(Stored_OTP, "", TextCompare: false) == 0 && !Module1.OTP_exit)
		{
			((Form)MyProject.Forms.GET_and_VERIFY_OTP).ShowDialog();
		}
		string left = Module1.get_single_value_NO_ARGS("LabourRtVry", "OtherSettings");
		if (Operators.CompareString(left, "1", TextCompare: false) != 0 && !Module1.OTP_exit)
		{
			((Form)MyProject.Forms.GET_and_VERIFY_OTP).ShowDialog();
		}
		if (Module1.OTP_exit)
		{
			return;
		}
		Show_Process_Label("Process Started...", "300");
		if (Operators.CompareString(Module1.frmperiod, "", TextCompare: false) == 0)
		{
			Module1.frmperiod = Conversions.ToString(DateAndTime.Today.Date.AddDays(-55.0));
			Module1.toperiod = Conversions.ToString(DateAndTime.Today.Date);
		}
		string text2 = Conversions.ToString(DateAndTime.Month(Conversions.ToDate(Module1.frmperiod)));
		string text3 = Conversions.ToString(DateAndTime.Year(Conversions.ToDate(Module1.frmperiod)));
		string inputStr = Conversions.ToString(DateAndTime.Month(Conversions.ToDate(Module1.toperiod)));
		string text4 = Conversions.ToString(DateAndTime.Year(Conversions.ToDate(Module1.toperiod)));
		double num2 = Conversion.Val(text2);
		double num3 = Conversion.Val(inputStr);
		for (double num4 = num2; num4 <= num3; num4++)
		{
			if (text2.Length == 1)
			{
				period1 = "0" + Conversions.ToString(num4) + text3;
			}
			else
			{
				period1 = Conversions.ToString(num4) + text3;
			}
			Module1.period_name = period1;
			text = "Select * from TmpTbl Where inv_dt Between #" + Module1.frmperiod + "# AND #" + Module1.toperiod + "#";
			da = new OleDbDataAdapter(text, Module1.con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			if (dt.Rows.Count <= 0)
			{
				((Control)this).Cursor = Module1.CreateCursor(cursor_file);
				BtnDownloadData_Click(RuntimeHelpers.GetObjectValue(sender), e);
				if (stop_process)
				{
					LBL_Progress.Text = "";
					PB1.Value = 0;
					((Control)PB1).Visible = false;
					((Control)this).Cursor = Cursors.Default;
					break;
				}
				Thread.Sleep(2000);
				btnJsonToExcel_Click(RuntimeHelpers.GetObjectValue(sender), e);
				Thread.Sleep(2000);
				btnExcelToAccess_Click(RuntimeHelpers.GetObjectValue(sender), e);
				BTNCompare_Click(RuntimeHelpers.GetObjectValue(sender), e);
				((Control)this).Cursor = Cursors.Default;
			}
			else
			{
				((Control)this).Cursor = Module1.CreateCursor(cursor_file);
				BTNCompare_Click(RuntimeHelpers.GetObjectValue(sender), e);
				((Control)this).Cursor = Cursors.Default;
			}
		}
	}

	private void upload_data_1(string url, string args)
	{
		Uri uri = new Uri(url);
		byte[] bytes = Encoding.UTF8.GetBytes(args);
		string text = SendRequest(uri, bytes, "application/json", "POST");
		if (Operators.CompareString(text, "", TextCompare: false) != 0)
		{
			string text2 = JsonConvert.SerializeObject(text);
		}
	}

	private string SendRequest_AUTH_TOCKEN(Uri uri, byte[] jsonDataBytes, string contentType, string method)
	{
		WebRequest webRequest = WebRequest.Create(uri);
		string value = "Bearer " + Module1.auth_Token;
		webRequest.Headers.Add("Authorization", value);
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

	private string SendRequest(Uri uri, byte[] jsonDataBytes, string contentType, string method)
	{
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

	private void Button1_Click(object sender, EventArgs e)
	{
		string path = Application.StartupPath + "\\Test.txt";
		FileStream fileStream = File.Create(path);
		byte[] bytes = new UTF8Encoding(encoderShouldEmitUTF8Identifier: true).GetBytes(TextBox1.Text);
		fileStream.Write(bytes, 0, bytes.Length);
		fileStream.Close();
	}

	private void BTN_GEN_Token_Click(object sender, EventArgs e)
	{
		Show_Process_Label("Token Generating...", "500");
		string text = "anilkamra2007@gmail.com";
		string text2 = "Anil@123";
		string text3 = "GHfUTjxGRuofVyqxNa";
		string text4 = "yA24uD9KtiHnvLm0BtxBAXFT";
		string text5 = "password";
		string s = "{\"username\":\"" + text + "\",\"password\":\"" + text2 + "\",\"client_id\":\"" + text3 + "\",\"client_secret\":\"" + text4 + "\",\"grant_type\":\"" + text5 + "\"}";
		string uriString = "https://pro.mastersindia.co/oauth/access_token";
		Uri uri = new Uri(uriString);
		byte[] bytes = Encoding.UTF8.GetBytes(s);
		string text6 = SendRequest(uri, bytes, "application/json", "POST");
		if (Operators.CompareString(text6, "", TextCompare: false) == 0)
		{
			Interaction.MsgBox("Token Not Generated");
		}
		else
		{
			string text7 = JsonConvert.SerializeObject(text6);
			JObject jObject = JObject.Parse(text6);
			Module1.auth_Token = jObject["access_token"].ToString().Replace("\"", "");
		}
		Show_Process_Label("Token Generated...", "500");
	}

	private void BtnDownloadData_Click(object sender, EventArgs e)
	{
		Show_Process_Label("Data Downloading... " + period1, "500");
		string text = "gstin=" + Module1.GSTIN1;
		string text2 = "&ret_period=" + period1;
		string requestUriString = "https://gstapi.in/taxpayerapis/returns/gstr2b?action=GET2B&" + text + text2;
		HttpWebRequest httpWebRequest = (HttpWebRequest)WebRequest.Create(requestUriString);
		if (Operators.CompareString(Module1.auth_Token, "", TextCompare: false) == 0)
		{
			Module1.auth_Token = "ef6599bdb1b1d091c56a5fbef6fab0af4b38f692";
		}
		string value = "Bearer " + Module1.auth_Token;
		httpWebRequest.Headers.Add("Authorization", value);
		httpWebRequest.Method = "GET";
		HttpWebResponse httpWebResponse = (HttpWebResponse)httpWebRequest.GetResponse();
		Stream responseStream = httpWebResponse.GetResponseStream();
		StreamReader streamReader = new StreamReader(responseStream, Encoding.GetEncoding("utf-8"));
		string text3 = streamReader.ReadToEnd();
		string text4 = text3;
		text4 = text4.Replace("[", "");
		text4 = text4.Replace("]", "");
		JObject jObject = JObject.Parse(text4);
		string left = jObject["status"].ToString().Replace("\"", "");
		if (Operators.CompareString(left, "0", TextCompare: false) == 0)
		{
			Interaction.MsgBox(jObject["message"].ToString());
			stop_process = true;
		}
		else
		{
			TextBox1.Text = jObject["data"].ToString();
			TextBox1.Text = TextBox1.Text.Replace("\"", "");
			TextBox1.Text = Base64Decode(TextBox1.Text);
			jObject = JObject.Parse(TextBox1.Text);
			string fnm = Application.StartupPath + "\\" + Module1.period_name + ".txt";
			WriteTxtFile(fnm, TextBox1.Text);
		}
		Show_Process_Label("Data Downloading Done ... " + period1, "500");
	}

	private void WriteTxtFile(string fnm, string data)
	{
		bool flag = File.Exists(fnm);
		using StreamWriter streamWriter = new StreamWriter(File.Open(fnm, FileMode.OpenOrCreate));
		streamWriter.WriteLine(data);
	}

	public static string Base64Decode(string base64EncodedData)
	{
		byte[] bytes = Convert.FromBase64String(base64EncodedData);
		return Encoding.UTF8.GetString(bytes);
	}

	private void btnJsonToExcel_Click(object sender, EventArgs e)
	{
		if (Operators.CompareString(TextBox1.Text, "", TextCompare: false) == 0)
		{
			Interaction.MsgBox("No Json To Convert", MsgBoxStyle.Critical);
			return;
		}
		Show_Process_Label("Processing ...", "500");
		Aspose.Cells.Workbook workbook = new Aspose.Cells.Workbook();
		Aspose.Cells.Worksheet worksheet = workbook.Worksheets[0];
		string text = TextBox1.Text;
		CellsFactory cellsFactory = new CellsFactory();
		Style style = cellsFactory.CreateStyle();
		style.HorizontalAlignment = TextAlignmentType.Center;
		style.Font.Color = Color.BlueViolet;
		style.Font.IsBold = true;
		JsonLayoutOptions jsonLayoutOptions = new JsonLayoutOptions();
		jsonLayoutOptions.TitleStyle = style;
		jsonLayoutOptions.ArrayAsTable = true;
		JsonUtility.ImportData(text, worksheet.Cells, 0, 0, jsonLayoutOptions);
		workbook.Save("Import-Data-JSON-To-Excel.xlsx");
		Show_Process_Label("Processing ...", "500");
	}

	public static List<JsonData> GetDataList(JObject obj)
	{
		List<JsonData> result = new List<JsonData>();
		foreach (JProperty item in (IEnumerable<JToken>)obj)
		{
		}
		return result;
	}

	private void btnExcelToAccess_Click(object sender, EventArgs e)
	{
		//IL_014f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0156: Expected Obj, but got Unknown
		//IL_0d60: Unknown result type (might be due to invalid IL or missing references)
		//IL_0d67: Expected Obj, but got Unknown
		Show_Process_Label("Processing ...", "500");
		string inputStr = Read_Excel_ColIndex();
		Application application = (Application)Activator.CreateInstance(Marshal.GetTypeFromCLSID(new Guid("00024500-0000-0000-C000-000000000046")));
		Microsoft.Office.Interop.Excel.Workbook workbook = application.Workbooks.Open(Application.StartupPath + "\\Import-Data-JSON-To-Excel.xlsx", RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
		Microsoft.Office.Interop.Excel.Worksheet worksheet = (Microsoft.Office.Interop.Excel.Worksheet)workbook.Worksheets["Sheet1"];
		string[] array = new string[20];
		int num = 0;
		int num2 = 0;
		bool flag = false;
		PB1.Maximum = 1000;
		string text = "delete from TmpTbl Where inv_dt Between #" + Module1.frmperiod + "# AND #" + Module1.toperiod + "#";
		OleDbCommand val = new OleDbCommand(text, Module1.con);
		val.ExecuteNonQuery();
		checked
		{
			int num3 = worksheet.Rows.Count - 1;
			for (int i = 7; i <= num3; i++)
			{
				DGVdata.RowCount += 1;
				string text2 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr)], null, "value", new object[0], null, null, null));
				string text3 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 1.0], null, "value", new object[0], null, null, null));
				string value = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 2.0], null, "value", new object[0], null, null, null));
				string value2 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 3.0], null, "value", new object[0], null, null, null));
				string text4 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 4.0], null, "value", new object[0], null, null, null));
				string left = text4;
				string value3 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 5.0], null, "value", new object[0], null, null, null));
				string text5 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 6.0], null, "value", new object[0], null, null, null));
				string left2 = text5;
				string value4 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 7.0], null, "value", new object[0], null, null, null));
				string value5 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 8.0], null, "value", new object[0], null, null, null));
				string value6 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 9.0], null, "value", new object[0], null, null, null));
				string value7 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 10.0], null, "value", new object[0], null, null, null));
				string value8 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 11.0], null, "value", new object[0], null, null, null));
				string value9 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 12.0], null, "value", new object[0], null, null, null));
				string text6 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 13.0], null, "value", new object[0], null, null, null));
				string value10 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 14.0], null, "value", new object[0], null, null, null));
				string value11 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 15.0], null, "value", new object[0], null, null, null));
				string value12 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 16.0], null, "value", new object[0], null, null, null));
				string value13 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 17.0], null, "value", new object[0], null, null, null));
				string value14 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 18.0], null, "value", new object[0], null, null, null));
				string value15 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 19.0], null, "value", new object[0], null, null, null));
				DGVdata.Rows[num2].Cells[0].Value = text2;
				DGVdata.Rows[num2].Cells[1].Value = text3;
				DGVdata.Rows[num2].Cells[2].Value = value;
				DGVdata.Rows[num2].Cells[3].Value = value2;
				DGVdata.Rows[num2].Cells[4].Value = text4;
				DGVdata.Rows[num2].Cells[5].Value = value3;
				DGVdata.Rows[num2].Cells[6].Value = text5;
				DGVdata.Rows[num2].Cells[7].Value = value4;
				DGVdata.Rows[num2].Cells[8].Value = value5;
				DGVdata.Rows[num2].Cells[9].Value = value6;
				DGVdata.Rows[num2].Cells[10].Value = value7;
				DGVdata.Rows[num2].Cells[11].Value = value8;
				DGVdata.Rows[num2].Cells[12].Value = value9;
				DGVdata.Rows[num2].Cells[13].Value = text6;
				DGVdata.Rows[num2].Cells[14].Value = value10;
				DGVdata.Rows[num2].Cells[15].Value = value11;
				DGVdata.Rows[num2].Cells[16].Value = value14;
				DGVdata.Rows[num2].Cells[17].Value = value15;
				DGVdata.Rows[num2].Cells[18].Value = value13;
				DGVdata.Rows[num2].Cells[19].Value = value12;
				if (Conversions.ToBoolean(Operators.OrObject(Operators.CompareObjectEqual(DGVdata.Rows[num2].Cells[0].Value, "", TextCompare: false), null)))
				{
					DGVdata.Rows[num2].Cells[0].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num2 - 1].Cells[0].Value);
					DGVdata.Rows[num2].Cells[1].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num2 - 1].Cells[1].Value);
					DGVdata.Rows[num2].Cells[2].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num2 - 1].Cells[2].Value);
					DGVdata.Rows[num2].Cells[3].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num2 - 1].Cells[3].Value);
				}
				int num4 = array.Length - 1;
				for (num = 0; num <= num4; num++)
				{
					array[num] = Conversions.ToString(DGVdata.Rows[num2].Cells[num].Value);
				}
				if (!((Operators.CompareString(text2, "", TextCompare: false) == 0) & (Operators.CompareString(text3, "", TextCompare: false) == 0) & (Operators.CompareString(left, "", TextCompare: false) == 0) & (Operators.CompareString(left2, "", TextCompare: false) == 0) & (Operators.CompareString(text6, "", TextCompare: false) == 0)))
				{
					if (Operators.CompareString(array[6], "", TextCompare: false) == 0)
					{
						array[6] = Conversions.ToString(DateAndTime.Today.Date);
					}
					string text7 = "Insert into TmpTbl (gst, party, ret_date, period, inv_no, types, inv_dt, amount, pos, rev  , itcavl, rsn, diffprcnt, num, rt, txval, igst, cess, cgst, sgst) values('" + array[0] + "','" + array[1] + "','" + array[2] + "','" + array[3] + "','" + array[4] + "','" + array[5] + "','" + array[6] + "','" + array[7] + "','" + array[8] + "','" + array[9] + "','" + array[10] + "','" + array[11] + "','" + array[12] + "','" + array[13] + "','" + array[14] + "','" + array[15] + "','" + array[16] + "','" + array[17] + "','" + array[18] + "','" + array[19] + "')";
					OleDbCommand val2 = new OleDbCommand(text7, Module1.con);
					val2.ExecuteNonQuery();
				}
				if ((Operators.CompareString(text2, "", TextCompare: false) == 0) & (Operators.CompareString(text3, "", TextCompare: false) == 0) & (Operators.CompareString(left, "", TextCompare: false) == 0) & (Operators.CompareString(left2, "", TextCompare: false) == 0) & (Operators.CompareString(text6, "", TextCompare: false) == 0))
				{
					break;
				}
				num2++;
				PB1.Value = (int)Math.Round(Conversion.Val(PB1.Value) + 1.0);
			}
			workbook.Close(RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
			application.Quit();
			releaseObject(application);
			releaseObject(workbook);
			releaseObject(worksheet);
			PB1.Value = 0;
			Show_Process_Label("Processing ...", "500");
		}
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

	private void BtnGetOTP_Click(object sender, EventArgs e)
	{
		//IL_000b: Unknown result type (might be due to invalid IL or missing references)
		((Form)MyProject.Forms.GET_and_VERIFY_OTP).ShowDialog();
	}

	private void BtnVerifyOTP_Click(object sender, EventArgs e)
	{
		Show_Process_Label("Verifying OTP...", "500");
		string text = "OTPVERIFY";
		string gSTIN = Module1.GSTIN1;
		string portalUserName = Module1.PortalUserName;
		string stored_OTP = Stored_OTP;
		string text2 = "1";
		string s = "{\"action\":\"" + text + "\",\"gstin\":\"" + gSTIN + "\",\"gst_username\":\"" + portalUserName + "\",\"OTP\":\"" + stored_OTP + "\",\"auth_extension\":\"" + text2 + "\"}";
		string uriString = "https://gstapi.in/taxpayerapis/authenticate";
		Uri uri = new Uri(uriString);
		byte[] bytes = Encoding.UTF8.GetBytes(s);
		string text3 = SendRequest_AUTH_TOCKEN(uri, bytes, "application/json", "POST");
		if (Operators.CompareString(text3, "", TextCompare: false) != 0)
		{
			string text4 = JsonConvert.SerializeObject(text3);
			JObject jObject = JObject.Parse(text3);
			string left = jObject["status"].ToString().Replace("\"", "");
			if (Operators.CompareString(left, "1", TextCompare: false) == 0)
			{
				Interaction.MsgBox(jObject["message"].ToString().Replace("\"", ""));
			}
			else
			{
				Interaction.MsgBox(jObject["message"].ToString().Replace("\"", ""));
			}
		}
		Show_Process_Label("OTP Verified...", "500");
	}

	private void Button2_Click(object sender, EventArgs e)
	{
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_000d: Invalid comparison between Unknown and I4
		if ((int)((CommonDialog)OpenFileDialog1).ShowDialog() == 1)
		{
			((FileDialog)OpenFileDialog1).InitialDirectory = "C:\\";
			((FileDialog)OpenFileDialog1).Title = "Select a JSON File";
			((FileDialog)OpenFileDialog1).Filter = "Json|*.json";
			txtPath.Text = ((FileDialog)OpenFileDialog1).FileName;
		}
	}

	private void Button3_Click(object sender, EventArgs e)
	{
		//IL_026c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0276: Expected Obj, but got Unknown
		if (RBAuto.Checked)
		{
			Whole_PROCESS(RuntimeHelpers.GetObjectValue(sender), e);
		}
		else
		{
			if (!RBManual.Checked)
			{
				return;
			}
			if (Operators.CompareString(txtPath.Text, "", TextCompare: false) == 0)
			{
				Interaction.MsgBox("Please Select Json File", MsgBoxStyle.Critical, "Bahi-Khata");
				return;
			}
			string text = ((ServerComputer)MyProject.Computer).FileSystem.ReadAllText(txtPath.Text);
			TextBox1.Text = text;
			string find = "\"\"";
			string replacement = "\"";
			string json = Strings.Replace(text, find, replacement);
			JObject jObject = JObject.Parse(json);
			string text2 = jObject["data"]["rtnprd"].ToString();
			text2 = text2.Replace("\"", "");
			if (Operators.CompareString(Module1.Bahi_khata_ret_period, text2, TextCompare: false) != 0)
			{
				Interaction.MsgBox("Please Select Return Period proper");
				return;
			}
			if (Operators.CompareString(Module1.frmperiod, "", TextCompare: false) == 0)
			{
				Module1.frmperiod = Conversions.ToString(DateAndTime.Today.Date.AddDays(-55.0));
				Module1.toperiod = Conversions.ToString(DateAndTime.Today.Date);
			}
			string text3 = Conversions.ToString(DateAndTime.Month(Conversions.ToDate(Module1.frmperiod)));
			string text4 = Conversions.ToString(DateAndTime.Year(Conversions.ToDate(Module1.frmperiod)));
			string inputStr = Conversions.ToString(DateAndTime.Month(Conversions.ToDate(Module1.toperiod)));
			string text5 = Conversions.ToString(DateAndTime.Year(Conversions.ToDate(Module1.toperiod)));
			double num = Conversion.Val(text3);
			double num2 = Conversion.Val(inputStr);
			for (double num3 = num; num3 <= num2; num3++)
			{
				if (text3.Length == 1)
				{
					period1 = "0" + Conversions.ToString(num3) + text4;
				}
				else
				{
					period1 = Conversions.ToString(num3) + text4;
				}
				Module1.period_name = period1;
				string text6 = "Select * from TmpTbl Where F7 Between #" + Module1.frmperiod + "# AND #" + Module1.toperiod + "#";
				da = new OleDbDataAdapter(text6, Module1.con);
				ds = new DataSet();
				((DbDataAdapter)(object)da).Fill(ds);
				dt = ds.Tables[0];
				if (dt.Rows.Count <= 0)
				{
					BtnDownloadData_Click(RuntimeHelpers.GetObjectValue(sender), e);
					if (stop_process)
					{
						LBL_Progress.Text = "";
						PB1.Value = 0;
						((Control)PB1).Visible = false;
						break;
					}
					Thread.Sleep(2000);
					btnJsonToExcel_Click(RuntimeHelpers.GetObjectValue(sender), e);
					Thread.Sleep(2000);
					btnExcelToAccess_Click(RuntimeHelpers.GetObjectValue(sender), e);
					BTNCompare_Click(RuntimeHelpers.GetObjectValue(sender), e);
				}
				else
				{
					BTNCompare_Click(RuntimeHelpers.GetObjectValue(sender), e);
				}
			}
		}
	}

	private void RBManual_CheckedChanged(object sender, EventArgs e)
	{
		if (RBManual.Checked)
		{
			((Control)txtPath).Visible = true;
			((Control)Button2).Visible = true;
		}
		else
		{
			((Control)txtPath).Visible = false;
			((Control)Button2).Visible = false;
		}
	}

	private void RBAuto_CheckedChanged(object sender, EventArgs e)
	{
	}

	private string Read_Excel_ColIndex()
	{
		Application application = (Application)Activator.CreateInstance(Marshal.GetTypeFromCLSID(new Guid("00024500-0000-0000-C000-000000000046")));
		Microsoft.Office.Interop.Excel.Workbook workbook = application.Workbooks.Open(Application.StartupPath + "\\Import-Data-JSON-To-Excel.xlsx", RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
		Microsoft.Office.Interop.Excel.Worksheet worksheet = (Microsoft.Office.Interop.Excel.Worksheet)workbook.Worksheets["Sheet1"];
		checked
		{
			int num = worksheet.Rows.Count - 1;
			for (int i = 1; i <= num; i++)
			{
				int num2 = 1;
				do
				{
					if (Operators.ConditionalCompareObjectEqual(NewLateBinding.LateGet(worksheet.Cells[i, num2], null, "value", new object[0], null, null, null), "docdata", TextCompare: false) && Operators.ConditionalCompareObjectEqual(NewLateBinding.LateGet(worksheet.Cells[i + 1, num2], null, "value", new object[0], null, null, null), "b2b", TextCompare: false))
					{
						return Conversions.ToString(num2);
					}
					num2++;
				}
				while (num2 <= 150);
			}
			string result = default;
			return result;
		}
	}

	private void Button4_Click(object sender, EventArgs e)
	{
		//IL_2931: Unknown result type (might be due to invalid IL or missing references)
		//IL_2938: Expected Obj, but got Unknown
		Show_Process_Label("Processing ...", "500");
		string inputStr = Read_Excel_ColIndex();
		Application application = (Application)Activator.CreateInstance(Marshal.GetTypeFromCLSID(new Guid("00024500-0000-0000-C000-000000000046")));
		Microsoft.Office.Interop.Excel.Workbook workbook = application.Workbooks.Open(Application.StartupPath + "\\Import-Data-JSON-To-Excel.xlsx", RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
		Microsoft.Office.Interop.Excel.Worksheet worksheet = (Microsoft.Office.Interop.Excel.Worksheet)workbook.Worksheets["Sheet1"];
		string[] array = new string[20];
		int num = 0;
		int num2 = 0;
		bool flag = false;
		PB1.Maximum = 1000;
		checked
		{
			int num3 = worksheet.Rows.Count - 1;
			string value = default;
			string value2 = default;
			string text3 = default;
			string value3 = default;
			string text4 = default;
			string value4 = default;
			string value5 = default;
			string value6 = default;
			string value7 = default;
			string value8 = default;
			string value9 = default;
			string value10 = default;
			string value11 = default;
			string text6 = default;
			string text7 = default;
			string text8 = default;
			string inputStr2 = default;
			string text5 = default;
			double num4 = default;
			double num5 = default;
			double num6 = default;
			double num7 = default;
			double num8 = default;
			for (int i = 7; i <= num3; i++)
			{
				string text;
				string text2;
				string left;
				string left2;
				if (flag)
				{
					flag = false;
					text = "";
					text2 = "";
					left = "";
					left2 = "";
				}
				else
				{
					text = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr)], null, "value", new object[0], null, null, null));
					text2 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 1.0], null, "value", new object[0], null, null, null));
					value = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 2.0], null, "value", new object[0], null, null, null));
					value2 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 3.0], null, "value", new object[0], null, null, null));
					text3 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 4.0], null, "value", new object[0], null, null, null));
					left = text3;
					value3 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 5.0], null, "value", new object[0], null, null, null));
					text4 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 6.0], null, "value", new object[0], null, null, null));
					left2 = text4;
					value4 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 7.0], null, "value", new object[0], null, null, null));
					value5 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 8.0], null, "value", new object[0], null, null, null));
					value6 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 9.0], null, "value", new object[0], null, null, null));
					value7 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 10.0], null, "value", new object[0], null, null, null));
					value8 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 11.0], null, "value", new object[0], null, null, null));
					value9 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 12.0], null, "value", new object[0], null, null, null));
					value10 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 13.0], null, "value", new object[0], null, null, null));
					value11 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 14.0], null, "value", new object[0], null, null, null));
					text5 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 15.0], null, "value", new object[0], null, null, null));
					text6 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 16.0], null, "value", new object[0], null, null, null));
					text7 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 17.0], null, "value", new object[0], null, null, null));
					text8 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 18.0], null, "value", new object[0], null, null, null));
					inputStr2 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 19.0], null, "value", new object[0], null, null, null));
				}
				string left3 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i + 1, Conversion.Val(inputStr)], null, "value", new object[0], null, null, null));
				string left4 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i + 1, Conversion.Val(inputStr) + 1.0], null, "value", new object[0], null, null, null));
				string left5 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i + 1, Conversion.Val(inputStr) + 4.0], null, "value", new object[0], null, null, null));
				string text9 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i + 1, Conversion.Val(inputStr) + 6.0], null, "value", new object[0], null, null, null));
				if (((Operators.CompareString(text, "", TextCompare: false) == 0) & (Operators.CompareString(text2, "", TextCompare: false) == 0) & (Operators.CompareString(left, "", TextCompare: false) == 0) & (Operators.CompareString(left2, "", TextCompare: false) == 0)) || ((Operators.CompareString(text5, null, TextCompare: false) == 0) & (Operators.CompareString(text6, null, TextCompare: false) == 0) & (Operators.CompareString(text7, null, TextCompare: false) == 0)))
				{
					break;
				}
				if (Operators.CompareString(text, "08AEQPK9575C1ZZ", TextCompare: false) == 0)
				{
					Console.WriteLine("testing");
				}
				if ((Operators.CompareString(left3, null, TextCompare: false) == 0) & (Operators.CompareString(left4, null, TextCompare: false) == 0))
				{
					if (Operators.CompareString(left5, null, TextCompare: false) == 0)
					{
						num4 = Conversion.Val(num4) + Conversion.Val(text5);
						num5 = Conversion.Val(num5) + Conversion.Val(text6);
						num6 = Conversion.Val(num6) + Conversion.Val(text7);
						num7 = Conversion.Val(num7) + Conversion.Val(text8);
						num8 = Conversion.Val(num8) + Conversion.Val(inputStr2);
						flag = true;
						continue;
					}
					num4 = Conversion.Val(num4) + Conversion.Val(text5);
					num5 = Conversion.Val(num5) + Conversion.Val(text6);
					num6 = Conversion.Val(num6) + Conversion.Val(text7);
					num7 = Conversion.Val(num7) + Conversion.Val(text8);
					num8 = Conversion.Val(num8) + Conversion.Val(inputStr2);
					DGVdata.RowCount += 1;
					if (Operators.CompareString(text, "", TextCompare: false) == 0)
					{
						int num9 = 0;
						do
						{
							DGVdata.Rows[num2].Cells[num9].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num2 - 1].Cells[num9].Value);
							num9++;
						}
						while (num9 <= 14);
						DGVdata.Rows[num2].Cells[4].Value = text3;
						DGVdata.Rows[num2].Cells[6].Value = text4;
						DGVdata.Rows[num2].Cells[7].Value = value4;
						DGVdata.Rows[num2].Cells[8].Value = value5;
					}
					else
					{
						DGVdata.Rows[num2].Cells[0].Value = text;
						DGVdata.Rows[num2].Cells[1].Value = text2;
						DGVdata.Rows[num2].Cells[2].Value = value;
						DGVdata.Rows[num2].Cells[3].Value = value2;
						DGVdata.Rows[num2].Cells[4].Value = text3;
						DGVdata.Rows[num2].Cells[5].Value = value3;
						DGVdata.Rows[num2].Cells[6].Value = text4;
						DGVdata.Rows[num2].Cells[7].Value = value4;
						DGVdata.Rows[num2].Cells[8].Value = value5;
						DGVdata.Rows[num2].Cells[9].Value = value6;
						DGVdata.Rows[num2].Cells[10].Value = value7;
						DGVdata.Rows[num2].Cells[11].Value = value8;
						DGVdata.Rows[num2].Cells[12].Value = value9;
						DGVdata.Rows[num2].Cells[13].Value = value10;
						DGVdata.Rows[num2].Cells[14].Value = value11;
					}
					DGVdata.Rows[num2].Cells[15].Value = num4;
					DGVdata.Rows[num2].Cells[16].Value = num5;
					DGVdata.Rows[num2].Cells[17].Value = num6;
					DGVdata.Rows[num2].Cells[18].Value = num8;
					DGVdata.Rows[num2].Cells[19].Value = num7;
					text = "";
					text2 = "";
					left = "";
					left2 = "";
					left3 = "";
					left4 = "";
					text9 = "";
					left5 = "";
					num4 = 0.0;
					num5 = 0.0;
					num6 = 0.0;
					num7 = 0.0;
					num8 = 0.0;
					num2++;
				}
				else
				{
					num4 = Conversion.Val(num4) + Conversion.Val(text5);
					num5 = Conversion.Val(num5) + Conversion.Val(text6);
					num6 = Conversion.Val(num6) + Conversion.Val(text7);
					num7 = Conversion.Val(num7) + Conversion.Val(text8);
					num8 = Conversion.Val(num8) + Conversion.Val(inputStr2);
					DGVdata.RowCount += 1;
					int num10 = num2 - 1;
					if (num10 < 0)
					{
						num10 = 0;
					}
					DGVdata.Rows[num2].Cells[0].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[0].Value);
					DGVdata.Rows[num2].Cells[1].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[1].Value);
					DGVdata.Rows[num2].Cells[2].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[2].Value);
					DGVdata.Rows[num2].Cells[3].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[3].Value);
					DGVdata.Rows[num2].Cells[4].Value = text3;
					DGVdata.Rows[num2].Cells[5].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[5].Value);
					DGVdata.Rows[num2].Cells[6].Value = text4;
					DGVdata.Rows[num2].Cells[7].Value = value4;
					DGVdata.Rows[num2].Cells[8].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[8].Value);
					DGVdata.Rows[num2].Cells[9].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[9].Value);
					DGVdata.Rows[num2].Cells[10].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[10].Value);
					DGVdata.Rows[num2].Cells[11].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[11].Value);
					DGVdata.Rows[num2].Cells[12].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[12].Value);
					DGVdata.Rows[num2].Cells[13].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[13].Value);
					DGVdata.Rows[num2].Cells[14].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num10].Cells[14].Value);
					DGVdata.Rows[num2].Cells[15].Value = num4;
					DGVdata.Rows[num2].Cells[16].Value = num5;
					DGVdata.Rows[num2].Cells[17].Value = num6;
					DGVdata.Rows[num2].Cells[18].Value = num8;
					DGVdata.Rows[num2].Cells[19].Value = num7;
					text = "";
					text2 = "";
					left = "";
					left2 = "";
					left3 = "";
					left4 = "";
					text9 = "";
					left5 = "";
					num4 = 0.0;
					num5 = 0.0;
					num6 = 0.0;
					num7 = 0.0;
					num8 = 0.0;
					num2++;
				}
			}
			PB1.Value = 0;
			string inputStr3 = Conversions.ToString(Conversion.Val(inputStr) + 24.0);
			int num11 = worksheet.Rows.Count - 1;
			string inputStr4 = default;
			for (int j = 7; j <= num11; j++)
			{
				string text;
				string text2;
				string left;
				string left2;
				if (flag)
				{
					flag = false;
					text = "";
					text2 = "";
					left = "";
					left2 = "";
				}
				else
				{
					text = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3)], null, "value", new object[0], null, null, null));
					text2 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 1.0], null, "value", new object[0], null, null, null));
					value = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 2.0], null, "value", new object[0], null, null, null));
					value2 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 3.0], null, "value", new object[0], null, null, null));
					text3 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 4.0], null, "value", new object[0], null, null, null));
					left = text3;
					value3 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 5.0], null, "value", new object[0], null, null, null));
					text4 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 6.0], null, "value", new object[0], null, null, null));
					left2 = text4;
					value4 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 7.0], null, "value", new object[0], null, null, null));
					value5 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 8.0], null, "value", new object[0], null, null, null));
					value6 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 9.0], null, "value", new object[0], null, null, null));
					value7 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 10.0], null, "value", new object[0], null, null, null));
					value8 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 11.0], null, "value", new object[0], null, null, null));
					value9 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 12.0], null, "value", new object[0], null, null, null));
					value10 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 13.0], null, "value", new object[0], null, null, null));
					value11 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 14.0], null, "value", new object[0], null, null, null));
					text5 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 15.0], null, "value", new object[0], null, null, null));
					text6 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 16.0], null, "value", new object[0], null, null, null));
					text7 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 17.0], null, "value", new object[0], null, null, null));
					text8 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 18.0], null, "value", new object[0], null, null, null));
					inputStr2 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j, Conversion.Val(inputStr3) + 19.0], null, "value", new object[0], null, null, null));
				}
				string left3 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j + 1, Conversion.Val(inputStr3)], null, "value", new object[0], null, null, null));
				string left4 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j + 1, Conversion.Val(inputStr3) + 1.0], null, "value", new object[0], null, null, null));
				string left5 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j + 1, Conversion.Val(inputStr3) + 4.0], null, "value", new object[0], null, null, null));
				string text9 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[j + 1, Conversion.Val(inputStr3) + 6.0], null, "value", new object[0], null, null, null));
				if (((Operators.CompareString(text, "", TextCompare: false) == 0) & (Operators.CompareString(text2, "", TextCompare: false) == 0) & (Operators.CompareString(left, "", TextCompare: false) == 0) & (Operators.CompareString(left2, "", TextCompare: false) == 0)) || ((Operators.CompareString(text6, null, TextCompare: false) == 0) & (Operators.CompareString(text7, null, TextCompare: false) == 0) & (Operators.CompareString(text8, null, TextCompare: false) == 0)))
				{
					break;
				}
				if ((Operators.CompareString(left3, null, TextCompare: false) == 0) & (Operators.CompareString(left4, null, TextCompare: false) == 0))
				{
					if (Operators.CompareString(left5, null, TextCompare: false) == 0)
					{
						num4 = Conversion.Val(num4) + Conversion.Val(text6);
						num5 = Conversion.Val(num5) + Conversion.Val(text7);
						num6 = Conversion.Val(num6) + Conversion.Val(text8);
						num7 = Conversion.Val(num7) + Conversion.Val(inputStr2);
						num8 = Conversion.Val(num8) + Conversion.Val(inputStr4);
						flag = true;
					}
					else
					{
						num4 = Conversion.Val(num4) + Conversion.Val(text6);
						num5 = Conversion.Val(num5) + Conversion.Val(text7);
						num6 = Conversion.Val(num6) + Conversion.Val(text8);
						num7 = Conversion.Val(num7) + Conversion.Val(inputStr2);
						num8 = Conversion.Val(num8) + Conversion.Val(inputStr4);
						DGVdata.RowCount += 1;
						if (Operators.CompareString(text, "", TextCompare: false) == 0)
						{
							int num12 = 0;
							do
							{
								DGVdata.Rows[num2].Cells[num12].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num2 - 1].Cells[num12].Value);
								num12++;
							}
							while (num12 <= 14);
							DGVdata.Rows[num2].Cells[4].Value = text3;
							DGVdata.Rows[num2].Cells[6].Value = text4;
							DGVdata.Rows[num2].Cells[7].Value = value4;
							DGVdata.Rows[num2].Cells[8].Value = value5;
						}
						else
						{
							DGVdata.Rows[num2].Cells[0].Value = text;
							DGVdata.Rows[num2].Cells[1].Value = text2;
							DGVdata.Rows[num2].Cells[2].Value = value;
							DGVdata.Rows[num2].Cells[3].Value = value2;
							DGVdata.Rows[num2].Cells[4].Value = text3;
							DGVdata.Rows[num2].Cells[5].Value = value3;
							DGVdata.Rows[num2].Cells[6].Value = text4;
							DGVdata.Rows[num2].Cells[7].Value = value4;
							DGVdata.Rows[num2].Cells[8].Value = value5;
							DGVdata.Rows[num2].Cells[9].Value = value6;
							DGVdata.Rows[num2].Cells[10].Value = value7;
							DGVdata.Rows[num2].Cells[11].Value = value8;
							DGVdata.Rows[num2].Cells[12].Value = value9;
							DGVdata.Rows[num2].Cells[13].Value = value10;
							DGVdata.Rows[num2].Cells[14].Value = value11;
						}
						DGVdata.Rows[num2].Cells[15].Value = num4;
						DGVdata.Rows[num2].Cells[16].Value = num5;
						DGVdata.Rows[num2].Cells[17].Value = num6;
						DGVdata.Rows[num2].Cells[18].Value = num8;
						DGVdata.Rows[num2].Cells[19].Value = num7;
						text = "";
						text2 = "";
						left = "";
						left2 = "";
						left3 = "";
						left4 = "";
						text9 = "";
						left5 = "";
						num4 = 0.0;
						num5 = 0.0;
						num6 = 0.0;
						num7 = 0.0;
						num8 = 0.0;
						num2++;
					}
				}
				else
				{
					num4 = Conversion.Val(num4) + Conversion.Val(text6);
					num5 = Conversion.Val(num5) + Conversion.Val(text7);
					num6 = Conversion.Val(num6) + Conversion.Val(text8);
					num7 = Conversion.Val(num7) + Conversion.Val(inputStr2);
					num8 = Conversion.Val(num8) + Conversion.Val(inputStr4);
					DGVdata.RowCount += 1;
					int num13 = num2 - 1;
					DGVdata.Rows[num2].Cells[0].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[0].Value);
					DGVdata.Rows[num2].Cells[1].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[1].Value);
					DGVdata.Rows[num2].Cells[2].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[2].Value);
					DGVdata.Rows[num2].Cells[3].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[3].Value);
					DGVdata.Rows[num2].Cells[4].Value = text3;
					DGVdata.Rows[num2].Cells[5].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[5].Value);
					DGVdata.Rows[num2].Cells[6].Value = text4;
					DGVdata.Rows[num2].Cells[7].Value = value4;
					DGVdata.Rows[num2].Cells[8].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[8].Value);
					DGVdata.Rows[num2].Cells[9].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[9].Value);
					DGVdata.Rows[num2].Cells[10].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[10].Value);
					DGVdata.Rows[num2].Cells[11].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[11].Value);
					DGVdata.Rows[num2].Cells[12].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[12].Value);
					DGVdata.Rows[num2].Cells[13].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[13].Value);
					DGVdata.Rows[num2].Cells[14].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num13].Cells[14].Value);
					DGVdata.Rows[num2].Cells[15].Value = num4;
					DGVdata.Rows[num2].Cells[16].Value = num5;
					DGVdata.Rows[num2].Cells[17].Value = num6;
					DGVdata.Rows[num2].Cells[18].Value = num8;
					DGVdata.Rows[num2].Cells[19].Value = num7;
					text = "";
					text2 = "";
					left = "";
					left2 = "";
					left3 = "";
					left4 = "";
					text9 = "";
					left5 = "";
					num4 = 0.0;
					num5 = 0.0;
					num6 = 0.0;
					num7 = 0.0;
					num8 = 0.0;
					num2++;
				}
				PB1.Value = (int)Math.Round(Conversion.Val(PB1.Value) + 1.0);
			}
			workbook.Close(RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
			application.Quit();
			releaseObject(application);
			releaseObject(workbook);
			releaseObject(worksheet);
			int num14 = DGVdata.Rows.Count - 2;
			for (int k = 0; k <= num14; k++)
			{
				int num15 = array.Length - 1;
				for (num = 0; num <= num15; num++)
				{
					array[num] = Conversions.ToString(DGVdata.Rows[k].Cells[num].Value);
				}
				if (Operators.CompareString(array[0], null, TextCompare: false) != 0)
				{
					string text10 = "Insert into TmpTbl (gst, party, ret_date, period, inv_no, types, inv_dt, amount, pos, rev  , itcavl, rsn, diffprcnt, num, rt, txval, igst, cess, cgst, sgst,period1) values('" + array[0] + "','" + array[1] + "','" + array[2] + "','" + array[3] + "','" + array[4] + "','" + array[5] + "','" + array[6] + "','" + array[7] + "','" + array[8] + "','" + array[9] + "','" + array[10] + "','" + array[11] + "','" + array[12] + "','" + array[13] + "','" + array[14] + "','" + array[15] + "','" + array[16] + "','" + array[17] + "','" + array[18] + "','" + array[19] + "','" + period1 + "')";
					OleDbCommand val = new OleDbCommand(text10, Module1.con);
					val.ExecuteNonQuery();
				}
				PB1.Value = (int)Math.Round(Conversion.Val(PB1.Value) + 1.0);
			}
			PB1.Value = 0;
			Show_Process_Label("Processing ...", "500");
		}
	}

	private void Button5_Click(object sender, EventArgs e)
	{
		Application.Exit();
	}

	private void Button6_Click(object sender, EventArgs e)
	{
		//IL_0e0f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0e16: Expected Obj, but got Unknown
		Show_Process_Label("Processing ...", "500");
		string inputStr = Read_Excel_ColIndex();
		Application application = (Application)Activator.CreateInstance(Marshal.GetTypeFromCLSID(new Guid("00024500-0000-0000-C000-000000000046")));
		Microsoft.Office.Interop.Excel.Workbook workbook = application.Workbooks.Open(Application.StartupPath + "\\Import-Data-JSON-To-Excel.xlsx", RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
		Microsoft.Office.Interop.Excel.Worksheet worksheet = (Microsoft.Office.Interop.Excel.Worksheet)workbook.Worksheets["Sheet1"];
		string[] array = new string[20];
		int num = 0;
		int num2 = 0;
		bool flag = false;
		PB1.Maximum = 1000;
		checked
		{
			int num3 = worksheet.Rows.Count - 1;
			for (int i = 7; i <= num3; i++)
			{
				DGVdata.RowCount += 1;
				string text = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr)], null, "value", new object[0], null, null, null));
				string text2 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 1.0], null, "value", new object[0], null, null, null));
				string text3 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 2.0], null, "value", new object[0], null, null, null));
				string text4 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 3.0], null, "value", new object[0], null, null, null));
				string text5 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 4.0], null, "value", new object[0], null, null, null));
				string left = text5;
				string text6 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 5.0], null, "value", new object[0], null, null, null));
				string text7 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 6.0], null, "value", new object[0], null, null, null));
				string left2 = text7;
				string text8 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 7.0], null, "value", new object[0], null, null, null));
				string text9 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 8.0], null, "value", new object[0], null, null, null));
				string text10 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 9.0], null, "value", new object[0], null, null, null));
				string text11 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 10.0], null, "value", new object[0], null, null, null));
				string text12 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 11.0], null, "value", new object[0], null, null, null));
				string text13 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 12.0], null, "value", new object[0], null, null, null));
				string text14 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 13.0], null, "value", new object[0], null, null, null));
				string text15 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 14.0], null, "value", new object[0], null, null, null));
				string text16 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 15.0], null, "value", new object[0], null, null, null));
				string text17 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 16.0], null, "value", new object[0], null, null, null));
				string text18 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 17.0], null, "value", new object[0], null, null, null));
				string text19 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 18.0], null, "value", new object[0], null, null, null));
				string text20 = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[i, Conversion.Val(inputStr) + 19.0], null, "value", new object[0], null, null, null));
				DGVdata.Rows[num2].Cells[0].Value = text;
				DGVdata.Rows[num2].Cells[1].Value = text2;
				DGVdata.Rows[num2].Cells[2].Value = text3;
				DGVdata.Rows[num2].Cells[3].Value = text4;
				DGVdata.Rows[num2].Cells[4].Value = text5;
				DGVdata.Rows[num2].Cells[5].Value = text6;
				DGVdata.Rows[num2].Cells[6].Value = text7;
				DGVdata.Rows[num2].Cells[7].Value = text8;
				DGVdata.Rows[num2].Cells[8].Value = text9;
				DGVdata.Rows[num2].Cells[9].Value = text10;
				DGVdata.Rows[num2].Cells[10].Value = text11;
				DGVdata.Rows[num2].Cells[11].Value = text12;
				DGVdata.Rows[num2].Cells[12].Value = text13;
				DGVdata.Rows[num2].Cells[13].Value = text14;
				DGVdata.Rows[num2].Cells[14].Value = text15;
				DGVdata.Rows[num2].Cells[15].Value = text16;
				DGVdata.Rows[num2].Cells[16].Value = text17;
				DGVdata.Rows[num2].Cells[17].Value = text18;
				DGVdata.Rows[num2].Cells[18].Value = text19;
				DGVdata.Rows[num2].Cells[19].Value = text20;
				if (Conversions.ToBoolean(Operators.OrObject(Operators.CompareObjectEqual(DGVdata.Rows[num2].Cells[0].Value, "", TextCompare: false), null)))
				{
					DGVdata.Rows[num2].Cells[0].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num2 - 1].Cells[0].Value);
					DGVdata.Rows[num2].Cells[1].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num2 - 1].Cells[1].Value);
					DGVdata.Rows[num2].Cells[2].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num2 - 1].Cells[2].Value);
					DGVdata.Rows[num2].Cells[3].Value = RuntimeHelpers.GetObjectValue(DGVdata.Rows[num2 - 1].Cells[3].Value);
				}
				int num4 = array.Length - 1;
				for (num = 0; num <= num4; num++)
				{
					array[num] = Conversions.ToString(DGVdata.Rows[num2].Cells[num].Value);
				}
				if (!((Operators.CompareString(text, "", TextCompare: false) == 0) & (Operators.CompareString(text2, "", TextCompare: false) == 0) & (Operators.CompareString(left, "", TextCompare: false) == 0) & (Operators.CompareString(left2, "", TextCompare: false) == 0)))
				{
					string text21 = "Insert into TmpTbl (gst, party, ret_date, period, inv_no, types, inv_dt, amount, pos, rev  , itcavl, rsn, diffprcnt, num, rt, txval, igst, cess, cgst, sgst) values('" + text + "','" + text2 + "','" + text3 + "','" + text4 + "','" + text5 + "','" + text6 + "','" + text7 + "','" + text8 + "','" + text9 + "','" + text10 + "','" + text11 + "','" + text12 + "','" + text13 + "','" + text14 + "','" + text15 + "','" + text16 + "','" + text17 + "','" + text18 + "','" + text19 + "','" + text20 + "')";
					text21 = "Insert into TmpTbl (gst, party, ret_date, period, inv_no, types, inv_dt, amount, pos, rev  , itcavl, rsn, diffprcnt, num, rt, txval, igst, cess, cgst, sgst) values('" + array[0] + "','" + array[1] + "','" + array[2] + "','" + array[3] + "','" + array[4] + "','" + array[5] + "','" + array[6] + "','" + array[7] + "','" + array[8] + "','" + array[9] + "','" + array[10] + "','" + array[11] + "','" + array[12] + "','" + array[13] + "','" + array[14] + "','" + array[15] + "','" + array[16] + "','" + array[17] + "','" + array[18] + "','" + array[19] + "')";
					OleDbCommand val = new OleDbCommand(text21, Module1.con);
					val.ExecuteNonQuery();
				}
				if ((Operators.CompareString(text, "", TextCompare: false) == 0) & (Operators.CompareString(text2, "", TextCompare: false) == 0) & (Operators.CompareString(left, "", TextCompare: false) == 0) & (Operators.CompareString(left2, "", TextCompare: false) == 0))
				{
					break;
				}
				num2++;
				PB1.Value = (int)Math.Round(Conversion.Val(PB1.Value) + 1.0);
			}
			workbook.Close(RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value), RuntimeHelpers.GetObjectValue(Missing.Value));
			application.Quit();
			releaseObject(application);
			releaseObject(workbook);
			releaseObject(worksheet);
			Interaction.MsgBox("Loaded");
		}
	}

	private string Gen_Query(string tbl)
	{
		if (Operators.CompareString(Module1.frmperiod, "", TextCompare: false) == 0)
		{
			Module1.frmperiod = Conversions.ToString(DateAndTime.Today.Date.AddDays(-55.0));
			Module1.toperiod = Conversions.ToString(DateAndTime.Today.Date);
		}
		string text = Conversions.ToString(DateAndTime.Month(Conversions.ToDate(Module1.frmperiod)));
		string text2 = Conversions.ToString(DateAndTime.Year(Conversions.ToDate(Module1.frmperiod)));
		string inputStr = Conversions.ToString(DateAndTime.Month(Conversions.ToDate(Module1.toperiod)));
		string text3 = Conversions.ToString(DateAndTime.Year(Conversions.ToDate(Module1.toperiod)));
		string[] array = new string[13];
		int num = 0;
		double num2 = Conversion.Val(text);
		double num3 = Conversion.Val(inputStr);
		checked
		{
			string text4 = default;
			for (double num4 = num2; num4 <= num3; num4++)
			{
				if (text.Length == 1)
				{
					array[num] = "0" + Conversions.ToString(num4) + text2;
				}
				else
				{
					array[num] = Conversions.ToString(num4) + text2;
				}
				text4 = ((num != 0) ? (text4 + "period1='" + array[num] + "' Or ") : ("period1='" + array[num] + "' Or "));
				num++;
			}
			string text5 = text4;
			string text6 = text5.Remove(text5.Length - 4, 3);
			return "select * from " + tbl + " where " + text6;
		}
	}

	private void BTNCompare_Click(object sender, EventArgs e)
	{
		//IL_00c2: Unknown result type (might be due to invalid IL or missing references)
		//IL_00cc: Expected Obj, but got Unknown
		//IL_017b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0185: Expected Obj, but got Unknown
		//IL_227a: Unknown result type (might be due to invalid IL or missing references)
		//IL_2284: Expected Obj, but got Unknown
		//IL_230c: Unknown result type (might be due to invalid IL or missing references)
		//IL_2316: Expected Obj, but got Unknown
		//IL_24f1: Unknown result type (might be due to invalid IL or missing references)
		//IL_24fb: Expected Obj, but got Unknown
		//IL_2608: Unknown result type (might be due to invalid IL or missing references)
		//IL_2612: Expected Obj, but got Unknown
		//IL_2752: Unknown result type (might be due to invalid IL or missing references)
		//IL_275c: Expected Obj, but got Unknown
		//IL_2869: Unknown result type (might be due to invalid IL or missing references)
		//IL_2873: Expected Obj, but got Unknown
		//IL_2eac: Unknown result type (might be due to invalid IL or missing references)
		//IL_2eb6: Expected Obj, but got Unknown
		//IL_29b3: Unknown result type (might be due to invalid IL or missing references)
		//IL_29bd: Expected Obj, but got Unknown
		//IL_2aca: Unknown result type (might be due to invalid IL or missing references)
		//IL_2ad4: Expected Obj, but got Unknown
		//IL_2c14: Unknown result type (might be due to invalid IL or missing references)
		//IL_2c1e: Expected Obj, but got Unknown
		//IL_2d2b: Unknown result type (might be due to invalid IL or missing references)
		//IL_2d35: Expected Obj, but got Unknown
		//IL_2ffd: Unknown result type (might be due to invalid IL or missing references)
		//IL_3007: Expected Obj, but got Unknown
		//IL_3257: Unknown result type (might be due to invalid IL or missing references)
		//IL_3261: Expected Obj, but got Unknown
		//IL_3153: Unknown result type (might be due to invalid IL or missing references)
		//IL_315d: Expected Obj, but got Unknown
		//IL_3a1b: Unknown result type (might be due to invalid IL or missing references)
		//IL_3a25: Expected Obj, but got Unknown
		string text = Application.StartupPath + "\\Prog.gif";
		if (File.Exists(text))
		{
			PictureBox1.Image = Image.FromFile(text);
			((Control)PictureBox1).Visible = true;
		}
		((Control)DGVFinal).Visible = false;
		match = 0.0;
		mismatch = 0.0;
		notinportal = 0.0;
		notinbahikhata = 0.0;
		string text2 = "select * from tempGSTR2A Where DocDate Between #" + Module1.frmperiod + "# AND #" + Module1.toperiod + "#";
		da = new OleDbDataAdapter(text2, Module1.con);
		ds = new DataSet();
		((DbDataAdapter)(object)da).Fill(ds);
		if (ds.Tables[0].Rows.Count > 0)
		{
			DataGridView dGVBahikhata_data = DGVBahikhata_data;
			dGVBahikhata_data.SelectionMode = (DataGridViewSelectionMode)1;
			dGVBahikhata_data.DataSource = ds.Tables[0];
			dGVBahikhata_data = null;
		}
		string text3 = "Select * from TmpTbl Where inv_dt Between #" + Module1.frmperiod + "# AND #" + Module1.toperiod + "#";
		text2 = text3;
		da = new OleDbDataAdapter(text2, Module1.con);
		ds = new DataSet();
		((DbDataAdapter)(object)da).Fill(ds);
		if (ds.Tables[0].Rows.Count > 0)
		{
			DataGridView dGVPortal_Data = DGVPortal_Data;
			dGVPortal_Data.SelectionMode = (DataGridViewSelectionMode)1;
			dGVPortal_Data.DataSource = ds.Tables[0];
			dGVPortal_Data = null;
		}
		LBL_Progress.Text = Conversions.ToString(DGVBahikhata_data.Rows.Count) + "\r\n" + Conversions.ToString(DGVPortal_Data.Rows.Count);
		DGVFinal.RowCount = 1;
		DGVFinal.ColumnCount = 13;
		int num = 0;
		PB1.Value = 0;
		checked
		{
			PB1.Maximum = (int)Math.Round(Conversion.Val(DGVBahikhata_data.Rows.Count));
			int num2 = DGVBahikhata_data.Rows.Count - 1;
			for (int i = 0; i <= num2 && !Operators.ConditionalCompareObjectEqual(DGVBahikhata_data.Rows[i].Cells[1].Value, "", TextCompare: false); i++)
			{
				string text4 = Conversions.ToString(DGVBahikhata_data.Rows[i].Cells[1].Value);
				string text5 = Conversions.ToString(DGVBahikhata_data.Rows[i].Cells[4].Value);
				string text6 = Conversions.ToString(DGVBahikhata_data.Rows[i].Cells[5].Value);
				string value = Conversions.ToString(DGVBahikhata_data.Rows[i].Cells[5].Value);
				string text7 = Conversions.ToString(DGVBahikhata_data.Rows[i].Cells[6].Value);
				string text8 = Conversions.ToString(DGVBahikhata_data.Rows[i].Cells[7].Value);
				string text9 = Conversions.ToString(DGVBahikhata_data.Rows[i].Cells[8].Value);
				string text10 = Conversions.ToString(DGVBahikhata_data.Rows[i].Cells[9].Value);
				string text11 = Conversions.ToString(DGVBahikhata_data.Rows[i].Cells[10].Value);
				string text12 = Conversions.ToString(DGVBahikhata_data.Rows[i].Cells[11].Value);
				string text13 = Conversions.ToString(Conversion.Val(text8) + Conversion.Val(text9) + Conversion.Val(text10) + Conversion.Val(text11) + Conversion.Val(text12));
				string value2 = Conversions.ToString(DGVBahikhata_data.Rows[i].Cells[2].Value);
				if (Operators.CompareString(text6, "", TextCompare: false) != 0 && (((double)text6.Length >= Conversion.Val(txtBillno.Text)) & (Conversion.Val(txtBillno.Text) > 0.0)))
				{
					text6 = Strings.Right(text6, (int)Math.Round(Conversion.Val(txtBillno.Text)));
				}
				if ((Operators.CompareString(text4, "08ABSPB7600G1ZQ", TextCompare: false) == 0) & (Operators.CompareString(text6, "658", TextCompare: false) == 0))
				{
					Console.WriteLine("Test");
				}
				int num3 = DGVPortal_Data.Rows.Count - 1;
				int num4 = 0;
				while (true)
				{
					if (num4 <= num3 && !Operators.ConditionalCompareObjectEqual(DGVPortal_Data.Rows[num4].Cells[1].Value, "", TextCompare: false))
					{
						string text14 = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[0].Value);
						string text15 = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[5].Value);
						switch (text15)
						{
						case "R":
							text15 = "Invoice";
							break;
						case "C":
							text15 = "Credit Note";
							break;
						case "D":
							text15 = "Debit Note";
							break;
						}
						string text16 = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[4].Value);
						string text17 = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[4].Value);
						string right = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[6].Value);
						string text18 = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[7].Value);
						string text19 = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[15].Value);
						string inputStr = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[16].Value);
						string inputStr2 = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[17].Value);
						string inputStr3 = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[19].Value);
						string inputStr4 = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[18].Value);
						string text20 = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[1].Value);
						string value3 = Conversions.ToString(DGVPortal_Data.Rows[num4].Cells[2].Value);
						if (Operators.CompareString(text16, "", TextCompare: false) != 0 && (((double)text16.Length >= Conversion.Val(txtBillno.Text)) & (Conversion.Val(txtBillno.Text) > 0.0)))
						{
							text16 = Strings.Right(text16, (int)Math.Round(Conversion.Val(txtBillno.Text)));
						}
						if ((Operators.CompareString(text14, "08ABSPB7600G1ZQ", TextCompare: false) == 0) & (Operators.CompareString(text16, "658", TextCompare: false) == 0) & (Operators.CompareString(text6, "658", TextCompare: false) == 0))
						{
							Console.WriteLine("Test");
						}
						if ((Operators.CompareString(text4, text14, TextCompare: false) == 0) & (Operators.CompareString(text7, right, TextCompare: false) == 0) & (Operators.CompareString(text6, text16, TextCompare: false) == 0) & (Operators.CompareString(text5, text15, TextCompare: false) == 0))
						{
							string inputStr5 = Conversions.ToString(Conversion.Val(text9) - Conversion.Val(inputStr));
							string inputStr6 = Conversions.ToString(Conversion.Val(text10) - Conversion.Val(inputStr3));
							string inputStr7 = Conversions.ToString(Conversion.Val(text11) - Conversion.Val(inputStr4));
							string inputStr8 = Conversions.ToString(Conversion.Val(text12) - Conversion.Val(inputStr2));
							if ((Conversion.Val(inputStr5) <= Conversion.Val(txtAmount.Text)) & (Conversion.Val(inputStr6) <= Conversion.Val(txtAmount.Text)) & (Conversion.Val(inputStr7) <= Conversion.Val(txtAmount.Text)) & (Conversion.Val(inputStr8) <= Conversion.Val(txtAmount.Text)))
							{
								DGVFinal.RowCount = (int)Math.Round(Conversion.Val(DGVFinal.RowCount) + 1.0);
								DGVFinal.Rows[num].Cells[0].Value = "Bills Matched";
								match = Conversion.Val(match) + 1.0;
								tot_match_Amt = Conversion.Val(tot_match_Amt) + Conversion.Val(text13);
								tot_match_taxable = Conversion.Val(tot_match_taxable) + Conversion.Val(text8);
								tot_match_IGST = Conversion.Val(tot_match_IGST) + Conversion.Val(text9);
								tot_match_CGST = Conversion.Val(tot_match_CGST) + Conversion.Val(text10);
								tot_match_SGST = Conversion.Val(tot_match_SGST) + Conversion.Val(text11);
								tot_match_CESS = Conversion.Val(tot_match_CESS) + Conversion.Val(text12);
								DGVFinal.Rows[num].Cells[1].Value = value2;
								DGVFinal.Rows[num].Cells[2].Value = text4;
								DGVFinal.Rows[num].Cells[3].Value = text5;
								DGVFinal.Rows[num].Cells[4].Value = text7;
								DGVFinal.Rows[num].Cells[5].Value = value;
								DGVFinal.Rows[num].Cells[6].Value = text13;
								DGVFinal.Rows[num].Cells[7].Value = text8;
								DGVFinal.Rows[num].Cells[8].Value = text9;
								DGVFinal.Rows[num].Cells[9].Value = text10;
								DGVFinal.Rows[num].Cells[10].Value = text11;
								DGVFinal.Rows[num].Cells[11].Value = text12;
								DGVFinal.Rows[num].Cells[12].Value = value3;
								num++;
							}
							else
							{
								DGVFinal.RowCount = (int)Math.Round(Conversion.Val(DGVFinal.RowCount) + 1.0);
								DGVFinal.Rows[num].Cells[0].Value = "Bills Found in both but mismatch";
								mismatch = Conversion.Val(mismatch) + 1.0;
								tot_mismatch_Amt = Conversion.Val(tot_mismatch_Amt) + Conversion.Val(text13);
								tot_mismatch_taxable = Conversion.Val(tot_mismatch_taxable) + Conversion.Val(text8);
								tot_mismatch_IGST = Conversion.Val(tot_mismatch_IGST) + Conversion.Val(text9);
								tot_mismatch_CGST = Conversion.Val(tot_mismatch_CGST) + Conversion.Val(text10);
								tot_mismatch_SGST = Conversion.Val(tot_mismatch_SGST) + Conversion.Val(text11);
								tot_mismatch_CESS = Conversion.Val(tot_mismatch_CESS) + Conversion.Val(text12);
								DGVFinal.Rows[num].Cells[1].Value = value2;
								DGVFinal.Rows[num].Cells[2].Value = text4;
								DGVFinal.Rows[num].Cells[3].Value = text5;
								DGVFinal.Rows[num].Cells[4].Value = text7;
								DGVFinal.Rows[num].Cells[5].Value = value;
								DGVFinal.Rows[num].Cells[6].Value = text13;
								DGVFinal.Rows[num].Cells[7].Value = text8;
								DGVFinal.Rows[num].Cells[8].Value = text9;
								DGVFinal.Rows[num].Cells[9].Value = text10;
								DGVFinal.Rows[num].Cells[10].Value = text11;
								DGVFinal.Rows[num].Cells[11].Value = text12;
								DGVFinal.Rows[num].Cells[12].Value = value3;
								num++;
							}
							break;
						}
						num4++;
						continue;
					}
					DGVFinal.RowCount = (int)Math.Round(Conversion.Val(DGVFinal.RowCount) + 1.0);
					DGVFinal.Rows[num].Cells[0].Value = "Bills in Bahi-Khata But Not Present at Portal";
					notinportal = Conversion.Val(notinportal) + 1.0;
					tot_notinportal_Amt = Conversion.Val(tot_notinportal_Amt) + Conversion.Val(text13);
					tot_notinportal_taxable = Conversion.Val(tot_notinportal_taxable) + Conversion.Val(text8);
					tot_notinportal_IGST = Conversion.Val(tot_notinportal_IGST) + Conversion.Val(text9);
					tot_notinportal_CGST = Conversion.Val(tot_notinportal_CGST) + Conversion.Val(text10);
					tot_notinportal_SGST = Conversion.Val(tot_notinportal_SGST) + Conversion.Val(text11);
					tot_notinportal_CESS = Conversion.Val(tot_notinportal_CESS) + Conversion.Val(text12);
					DGVFinal.Rows[num].Cells[1].Value = value2;
					DGVFinal.Rows[num].Cells[2].Value = text4;
					DGVFinal.Rows[num].Cells[3].Value = text5;
					DGVFinal.Rows[num].Cells[4].Value = text7;
					DGVFinal.Rows[num].Cells[5].Value = value;
					DGVFinal.Rows[num].Cells[6].Value = text13;
					DGVFinal.Rows[num].Cells[7].Value = text8;
					DGVFinal.Rows[num].Cells[8].Value = text9;
					DGVFinal.Rows[num].Cells[9].Value = text10;
					DGVFinal.Rows[num].Cells[10].Value = text11;
					DGVFinal.Rows[num].Cells[11].Value = text12;
					num++;
					break;
				}
				Application.DoEvents();
				PB1.Value = (int)Math.Round(Conversion.Val(PB1.Value) + 1.0);
				LBL_Progress.Text = "Preparing Data ";
				Thread.Sleep(25);
			}
			PB1.Value = 0;
			LBL_Progress.Text = "Data Prepared 100 %";
			PB1.Value = 0;
			PB1.Maximum = (int)Math.Round(Conversion.Val(DGVPortal_Data.Rows.Count));
			int num5 = DGVPortal_Data.Rows.Count - 1;
			for (int j = 0; j <= num5 && !Operators.ConditionalCompareObjectEqual(DGVPortal_Data.Rows[j].Cells[1].Value, "", TextCompare: false); j++)
			{
				string text21 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[0].Value);
				string text22 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[5].Value);
				string text23 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[4].Value);
				string value4 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[4].Value);
				string text24 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[6].Value);
				string text25 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[15].Value);
				string text26 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[16].Value);
				string text27 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[17].Value);
				string text28 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[18].Value);
				string text29 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[19].Value);
				string text30 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[7].Value);
				string value5 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[1].Value);
				string value6 = Conversions.ToString(DGVPortal_Data.Rows[j].Cells[2].Value);
				switch (text22)
				{
				case "R":
					text22 = "Invoice";
					break;
				case "C":
					text22 = "Credit Note";
					break;
				case "D":
					text22 = "Debit Note";
					break;
				}
				if (Operators.CompareString(text23, "", TextCompare: false) != 0 && (double)text23.Length >= Conversion.Val(txtBillno.Text))
				{
					text23 = Strings.Right(text23, (int)Math.Round(Conversion.Val(txtBillno.Text)));
				}
				int num6 = DGVBahikhata_data.Rows.Count - 1;
				int num7 = 0;
				while (true)
				{
					if (num7 <= num6 && !Operators.ConditionalCompareObjectEqual(DGVBahikhata_data.Rows[num7].Cells[1].Value, "", TextCompare: false))
					{
						string left = Conversions.ToString(DGVBahikhata_data.Rows[num7].Cells[1].Value);
						string left2 = Conversions.ToString(DGVBahikhata_data.Rows[num7].Cells[4].Value);
						string text31 = Conversions.ToString(DGVBahikhata_data.Rows[num7].Cells[5].Value);
						string text32 = Conversions.ToString(DGVBahikhata_data.Rows[num7].Cells[5].Value);
						string left3 = Conversions.ToString(DGVBahikhata_data.Rows[num7].Cells[6].Value);
						string inputStr9 = Conversions.ToString(DGVBahikhata_data.Rows[num7].Cells[7].Value);
						string inputStr10 = Conversions.ToString(DGVBahikhata_data.Rows[num7].Cells[8].Value);
						string inputStr11 = Conversions.ToString(DGVBahikhata_data.Rows[num7].Cells[9].Value);
						string inputStr12 = Conversions.ToString(DGVBahikhata_data.Rows[num7].Cells[10].Value);
						string inputStr13 = Conversions.ToString(DGVBahikhata_data.Rows[num7].Cells[11].Value);
						string text33 = Conversions.ToString(Conversion.Val(inputStr9) + Conversion.Val(inputStr10) + Conversion.Val(inputStr11) + Conversion.Val(inputStr12) + Conversion.Val(inputStr13));
						string text34 = Conversions.ToString(DGVBahikhata_data.Rows[num7].Cells[2].Value);
						if (Operators.CompareString(text31, "", TextCompare: false) != 0 && (double)text31.Length >= Conversion.Val(txtBillno.Text))
						{
							text31 = Strings.Right(text31, (int)Math.Round(Conversion.Val(txtBillno.Text)));
						}
						if ((Operators.CompareString(left, text21, TextCompare: false) == 0) & (Operators.CompareString(left3, text24, TextCompare: false) == 0) & (Operators.CompareString(text31, text23, TextCompare: false) == 0) & (Operators.CompareString(left2, text22, TextCompare: false) == 0))
						{
							break;
						}
						num7++;
						continue;
					}
					DGVFinal.RowCount = (int)Math.Round(Conversion.Val(DGVFinal.RowCount) + 1.0);
					DGVFinal.Rows[num].Cells[0].Value = "Bills Present at Portal but not found in bahi-khata";
					notinbahikhata = Conversion.Val(notinbahikhata) + 1.0;
					tot_notinbahikhata_Amt = Conversion.Val(tot_notinbahikhata_Amt) + Conversion.Val(text30);
					tot_notinbahikhata_taxable = Conversion.Val(tot_notinbahikhata_taxable) + Conversion.Val(text25);
					tot_notinbahikhata_IGST = Conversion.Val(tot_notinbahikhata_IGST) + Conversion.Val(text26);
					tot_notinbahikhata_CGST = Conversion.Val(tot_notinbahikhata_CGST) + Conversion.Val(text27);
					tot_notinbahikhata_SGST = Conversion.Val(tot_notinbahikhata_SGST) + Conversion.Val(text28);
					tot_notinbahikhata_CESS = Conversion.Val(tot_notinbahikhata_CESS) + Conversion.Val(text29);
					DGVFinal.Rows[num].Cells[1].Value = value5;
					DGVFinal.Rows[num].Cells[2].Value = text21;
					DGVFinal.Rows[num].Cells[3].Value = text22;
					DGVFinal.Rows[num].Cells[4].Value = text24;
					DGVFinal.Rows[num].Cells[5].Value = value4;
					DGVFinal.Rows[num].Cells[6].Value = text30;
					DGVFinal.Rows[num].Cells[7].Value = text25;
					DGVFinal.Rows[num].Cells[8].Value = text26;
					DGVFinal.Rows[num].Cells[9].Value = text27;
					DGVFinal.Rows[num].Cells[10].Value = text28;
					DGVFinal.Rows[num].Cells[11].Value = text29;
					DGVFinal.Rows[num].Cells[12].Value = value6;
					num++;
					break;
				}
				Application.DoEvents();
				PB1.Value = (int)Math.Round(Conversion.Val(PB1.Value) + 1.0);
				LBL_Progress.Text = "Finishing Results " + Conversions.ToString(PB1.Value) + " %";
				Thread.Sleep(25);
			}
			LBL_Progress.Text = "Results Produced 100 %";
			DGVFinal.Sort(DGVFinal.Columns[0], ListSortDirection.Ascending);
			int num8 = DGVFinal.Rows.Count - 1;
			for (int k = num8; k >= 0; k += -1)
			{
				int num9 = k - 1;
				for (int l = num9; l >= 0; l += -1)
				{
					if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[k].Cells[0].Value, DGVFinal.Rows[l].Cells[0].Value, TextCompare: false))
					{
						DGVFinal.Rows[k].Cells[0].Value = "";
						break;
					}
				}
			}
			DGVFinal.Columns[0].Width = Conversions.ToInteger("250");
			DGVFinal.Columns[1].Width = Conversions.ToInteger("250");
			DGVFinal.Columns[2].Width = Conversions.ToInteger("170");
			DGVFinal.Columns[3].Width = Conversions.ToInteger("160");
			if (Conversion.Val(DGVPortal_Data.ColumnCount) > 0.0)
			{
				DGVPortal_Data.Columns[0].Width = Conversions.ToInteger("150");
				DGVPortal_Data.Columns[1].Width = Conversions.ToInteger("200");
				DGVPortal_Data.Columns[4].Width = Conversions.ToInteger("130");
			}
			if (Conversion.Val(DGVBahikhata_data.ColumnCount) > 0.0)
			{
				DGVBahikhata_data.Columns[1].Width = Conversions.ToInteger("150");
				DGVBahikhata_data.Columns[2].Width = Conversions.ToInteger("200");
				DGVBahikhata_data.Columns[5].Width = Conversions.ToInteger("130");
				DGVBahikhata_data.Columns[6].Width = Conversions.ToInteger("120");
				DGVBahikhata_data.Columns[7].Width = Conversions.ToInteger("120");
				DGVBahikhata_data.Columns[8].Width = Conversions.ToInteger("120");
				DGVBahikhata_data.Columns[9].Width = Conversions.ToInteger("120");
				DGVBahikhata_data.Columns[10].Width = Conversions.ToInteger("120");
				DGVBahikhata_data.Columns[11].Width = Conversions.ToInteger("120");
			}
			DGVFinal.Columns[0].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)1;
			DGVFinal.Columns[1].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)1;
			DGVFinal.Columns[2].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)1;
			DGVFinal.Columns[3].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)1;
			DGVFinal.Columns[4].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)1;
			DGVFinal.Columns[5].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)4;
			DGVFinal.Columns[6].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)4;
			DGVFinal.Columns[7].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)4;
			DGVFinal.Columns[8].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)4;
			DGVFinal.Columns[9].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)4;
			DGVFinal.Columns[10].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)4;
			DGVFinal.Columns[11].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)4;
			DGVFinal.Columns[12].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)4;
			double num10 = Conversion.Val(notinportal) + Conversion.Val(notinbahikhata) + Conversion.Val(mismatch) + Conversion.Val(match);
			string[] array = new string[6] { "GSTR-2 Match Summary >>", "Bills Found (But Mismatch)", "Bills Found in Bahi-Khata (But Not at Portal)", "Bills Found at Portal (But Not in Bahi-Khata)", "Bills Matched", "Total Bills" };
			DGVFinal.Rows.Insert(0, (object[])array);
			DGVFinal.Rows[0].DefaultCellStyle.Font = new Font("Verdana", 7f, (FontStyle)1);
			string[] array2 = new string[6]
			{
				"",
				Conversions.ToString(mismatch),
				Conversions.ToString(notinportal),
				Conversions.ToString(notinbahikhata),
				Conversions.ToString(match),
				Conversions.ToString(num10)
			};
			DGVFinal.Rows.Insert(1, (object[])array2);
			DGVFinal.Rows[1].DefaultCellStyle.Font = new Font("Verdana", 7f, (FontStyle)1);
			DGVFinal.DefaultCellStyle.WrapMode = (DataGridViewTriState)1;
			DGVFinal.Rows[0].Height = 65;
			DGVFinal.Rows[0].DefaultCellStyle.BackColor = Color.LightGreen;
			DGVFinal.Rows[1].DefaultCellStyle.BackColor = Color.Yellow;
			string[] array3 = new string[3] { "", "", "" };
			DGVFinal.Rows.Insert(2, (object[])array3);
			bool flag = false;
			bool flag2 = false;
			bool flag3 = false;
			bool flag4 = false;
			int num11 = DGVFinal.Rows.Count - 1;
			for (int m = 0; m <= num11; m++)
			{
				if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[m].Cells[0].Value, "Bills in Bahi-Khata But Not Present at Portal", TextCompare: false) && !flag)
				{
					flag = true;
					string[] array4 = new string[3] { "", "", "" };
					DGVFinal.Rows.Insert(m, (object[])array4);
					string[] array5 = new string[5]
					{
						"",
						"Bills in Bahi-Khata But Not Present at Portal",
						"",
						"",
						Conversions.ToString(notinportal)
					};
					DGVFinal.Rows.Insert(m + 1, (object[])array5);
					DGVFinal.Rows[m + 1].DefaultCellStyle.BackColor = Color.Yellow;
					DGVFinal.Rows[m + 1].DefaultCellStyle.Font = new Font("Verdana", 7f, (FontStyle)1);
					DGVFinal.Rows[m + 1].Height = 35;
					string[] array6 = new string[13]
					{
						"", "Party Name", "GSTIN", "Type", "Bill Date", "Bill No", "Bill Amount", "Taxable", "IGST", "CGST",
						"SGST", "CESS", "Return Date"
					};
					DGVFinal.Rows.Insert(m + 2, (object[])array6);
					DGVFinal.Rows[m + 2].Height = 45;
					DGVFinal.Rows[m + 2].DefaultCellStyle.BackColor = Color.Pink;
					DGVFinal.Rows[m + 2].DefaultCellStyle.Font = new Font("Verdana", 7f, (FontStyle)1);
					DGVFinal.Rows[m + 2].Cells[0].Value = "";
				}
				if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[m].Cells[0].Value, "Bills Present at Portal but not found in bahi-khata", TextCompare: false) && !flag2)
				{
					flag2 = true;
					string[] array7 = new string[3] { "", "", "" };
					DGVFinal.Rows.Insert(m, (object[])array7);
					string[] array8 = new string[5]
					{
						"",
						"Bills Present at Portal but not found in bahi-khata",
						"",
						"",
						Conversions.ToString(notinbahikhata)
					};
					DGVFinal.Rows.Insert(m + 1, (object[])array8);
					DGVFinal.Rows[m + 1].DefaultCellStyle.BackColor = Color.Yellow;
					DGVFinal.Rows[m + 1].DefaultCellStyle.Font = new Font("Verdana", 7f, (FontStyle)1);
					DGVFinal.Rows[m + 1].Height = 45;
					string[] array9 = new string[13]
					{
						"", "Party Name", "GSTIN", "Type", "Bill Date", "Bill No", "Bill Amount", "Taxable", "IGST", "CGST",
						"SGST", "CESS", "Return Date"
					};
					DGVFinal.Rows.Insert(m + 2, (object[])array9);
					DGVFinal.Rows[m + 2].Height = 45;
					DGVFinal.Rows[m + 2].DefaultCellStyle.BackColor = Color.Pink;
					DGVFinal.Rows[m + 2].DefaultCellStyle.Font = new Font("Verdana", 7f, (FontStyle)1);
					DGVFinal.Rows[m + 2].Cells[0].Value = "";
				}
				if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[m].Cells[0].Value, "Bills Found in both but mismatch", TextCompare: false) && !flag3)
				{
					flag3 = true;
					string[] array10 = new string[3] { "", "", "" };
					DGVFinal.Rows.Insert(m, (object[])array10);
					string[] array11 = new string[5]
					{
						"",
						"Bills Found in both but mismatch",
						"",
						"",
						Conversions.ToString(mismatch)
					};
					DGVFinal.Rows.Insert(m + 1, (object[])array11);
					DGVFinal.Rows[m + 1].DefaultCellStyle.BackColor = Color.Yellow;
					DGVFinal.Rows[m + 1].DefaultCellStyle.Font = new Font("Verdana", 7f, (FontStyle)1);
					DGVFinal.Rows[m + 1].Height = 45;
					string[] array12 = new string[13]
					{
						"", "Party Name", "GSTIN", "Type", "Bill Date", "Bill No", "Bill Amount", "Taxable", "IGST", "CGST",
						"SGST", "CESS", "Return Date"
					};
					DGVFinal.Rows.Insert(m + 2, (object[])array12);
					DGVFinal.Rows[m + 2].Height = 45;
					DGVFinal.Rows[m + 2].DefaultCellStyle.BackColor = Color.Pink;
					DGVFinal.Rows[m + 2].DefaultCellStyle.Font = new Font("Verdana", 7f, (FontStyle)1);
					DGVFinal.Rows[m + 2].Cells[0].Value = "";
				}
				if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[m].Cells[0].Value, "Bills Matched", TextCompare: false) && !flag4)
				{
					flag4 = true;
					string[] array13 = new string[3] { "", "", "" };
					DGVFinal.Rows.Insert(m, (object[])array13);
					string[] array14 = new string[5]
					{
						"",
						"Bills Matched",
						"",
						"",
						Conversions.ToString(match)
					};
					DGVFinal.Rows.Insert(m + 1, (object[])array14);
					DGVFinal.Rows[m + 1].DefaultCellStyle.BackColor = Color.Yellow;
					DGVFinal.Rows[m + 1].DefaultCellStyle.Font = new Font("Verdana", 7f, (FontStyle)1);
					DGVFinal.Rows[m + 1].Height = 45;
					string[] array15 = new string[13]
					{
						"", "Party Name", "GSTIN", "Type", "Bill Date", "Bill No", "Bill Amount", "Taxable", "IGST", "CGST",
						"SGST", "CESS", "Return Date"
					};
					DGVFinal.Rows.Insert(m + 2, (object[])array15);
					DGVFinal.Rows[m + 2].Height = 45;
					DGVFinal.Rows[m + 2].DefaultCellStyle.BackColor = Color.Pink;
					DGVFinal.Rows[m + 2].DefaultCellStyle.Font = new Font("Verdana", 7f, (FontStyle)1);
					DGVFinal.Rows[m + 2].Cells[0].Value = "";
				}
			}
			int num12 = DGVFinal.Rows.Count - 1;
			int num13 = 0;
			if (num13 <= num12 && Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[num13].Cells[1].Value, "Bills in Bahi-Khata But Not Present at Portal", TextCompare: false))
			{
				string[] array16 = new string[12]
				{
					"",
					"Total : ",
					"",
					"",
					"",
					"",
					Conversions.ToString(tot_mismatch_Amt),
					Conversions.ToString(tot_mismatch_taxable),
					Conversions.ToString(tot_mismatch_IGST),
					Conversions.ToString(tot_mismatch_CGST),
					Conversions.ToString(tot_mismatch_SGST),
					Conversions.ToString(tot_mismatch_CESS)
				};
				DGVFinal.Rows.Insert(num13 - 1, (object[])array16);
				DGVFinal.Rows[num13 - 1].Height = 35;
				DGVFinal.Rows[num13 - 1].DefaultCellStyle.Font = new Font("Verdana", 6f, (FontStyle)1);
			}
			int num14 = DGVFinal.Rows.Count - 1;
			for (int n = 0; n <= num14; n++)
			{
				if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[n].Cells[1].Value, "Bills Matched", TextCompare: false))
				{
					string[] array17 = new string[12]
					{
						"",
						"Total : ",
						"",
						"",
						"",
						"",
						Conversions.ToString(tot_notinportal_Amt),
						Conversions.ToString(tot_notinportal_taxable),
						Conversions.ToString(tot_notinportal_IGST),
						Conversions.ToString(tot_notinportal_CGST),
						Conversions.ToString(tot_notinportal_SGST),
						Conversions.ToString(tot_notinportal_CESS)
					};
					DGVFinal.Rows.Insert(n - 1, (object[])array17);
					DGVFinal.Rows[n - 1].Height = 35;
					DGVFinal.Rows[n - 1].DefaultCellStyle.Font = new Font("Verdana", 6f, (FontStyle)1);
					break;
				}
			}
			int num15 = DGVFinal.Rows.Count - 1;
			for (int num16 = 0; num16 <= num15; num16++)
			{
				if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[num16].Cells[1].Value, "Bills Present at Portal but not found in bahi-khata", TextCompare: false))
				{
					string[] array18 = new string[12]
					{
						"",
						"Total : ",
						"",
						"",
						"",
						"",
						Conversions.ToString(tot_match_Amt),
						Conversions.ToString(tot_match_taxable),
						Conversions.ToString(tot_match_IGST),
						Conversions.ToString(tot_match_CGST),
						Conversions.ToString(tot_match_SGST),
						Conversions.ToString(tot_match_CESS)
					};
					DGVFinal.Rows.Insert(num16 - 1, (object[])array18);
					DGVFinal.Rows[num16 - 1].Height = 35;
					DGVFinal.Rows[num16 - 1].DefaultCellStyle.Font = new Font("Verdana", 6f, (FontStyle)1);
					break;
				}
			}
			string[] array19 = new string[12]
			{
				"",
				"Total : ",
				"",
				"",
				"",
				"",
				Conversions.ToString(tot_notinbahikhata_Amt),
				Conversions.ToString(tot_notinbahikhata_taxable),
				Conversions.ToString(tot_notinbahikhata_IGST),
				Conversions.ToString(tot_notinbahikhata_CGST),
				Conversions.ToString(tot_notinbahikhata_SGST),
				Conversions.ToString(tot_notinbahikhata_CESS)
			};
			DGVFinal.Rows.Insert(DGVFinal.Rows.Count - 1, (object[])array19);
			DGVFinal.Rows[DGVFinal.Rows.Count - 2].DefaultCellStyle.Font = new Font("Verdana", 6f, (FontStyle)1);
			int num17 = DGVFinal.Rows.Count - 1;
			for (int num18 = 0; num18 <= num17; num18++)
			{
				if (!Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[num18].Cells[0].Value, "Bills in Bahi-Khata But Not Present at Portal", TextCompare: false))
				{
					continue;
				}
				int num19 = num18;
				int num20 = DGVFinal.Rows.Count - 1;
				for (int num21 = num19; num21 <= num20; num21++)
				{
					if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[num21].Cells[1].Value, "Total : ", TextCompare: false))
					{
						DGVFinal.Rows[num21].Cells[6].Value = tot_notinportal_Amt;
						DGVFinal.Rows[num21].Cells[7].Value = tot_notinportal_taxable;
						DGVFinal.Rows[num21].Cells[8].Value = tot_notinportal_IGST;
						DGVFinal.Rows[num21].Cells[9].Value = tot_notinportal_CGST;
						DGVFinal.Rows[num21].Cells[10].Value = tot_notinportal_SGST;
						DGVFinal.Rows[num21].Cells[11].Value = tot_notinportal_CESS;
						break;
					}
				}
			}
			int num22 = DGVFinal.Rows.Count - 1;
			for (int num23 = 0; num23 <= num22; num23++)
			{
				if (!Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[num23].Cells[0].Value, "Bills Matched", TextCompare: false))
				{
					continue;
				}
				int num24 = num23;
				int num25 = DGVFinal.Rows.Count - 1;
				for (int num26 = num24; num26 <= num25; num26++)
				{
					if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[num26].Cells[1].Value, "Total : ", TextCompare: false))
					{
						DGVFinal.Rows[num26].Cells[6].Value = tot_match_Amt;
						DGVFinal.Rows[num26].Cells[7].Value = tot_match_taxable;
						DGVFinal.Rows[num26].Cells[8].Value = tot_match_IGST;
						DGVFinal.Rows[num26].Cells[9].Value = tot_match_CGST;
						DGVFinal.Rows[num26].Cells[10].Value = tot_match_SGST;
						DGVFinal.Rows[num26].Cells[11].Value = tot_match_CESS;
						break;
					}
				}
			}
			int num27 = DGVFinal.Rows.Count - 1;
			for (int num28 = 0; num28 <= num27; num28++)
			{
				if (!Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[num28].Cells[0].Value, "Bills Present at Portal but not found in bahi-khata", TextCompare: false))
				{
					continue;
				}
				int num29 = num28;
				int num30 = DGVFinal.Rows.Count - 1;
				for (int num31 = num29; num31 <= num30; num31++)
				{
					if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[num31].Cells[1].Value, "Total : ", TextCompare: false))
					{
						DGVFinal.Rows[num31].Cells[6].Value = tot_notinbahikhata_Amt;
						DGVFinal.Rows[num31].Cells[7].Value = tot_notinbahikhata_taxable;
						DGVFinal.Rows[num31].Cells[8].Value = tot_notinbahikhata_IGST;
						DGVFinal.Rows[num31].Cells[9].Value = tot_notinbahikhata_CGST;
						DGVFinal.Rows[num31].Cells[10].Value = tot_notinbahikhata_SGST;
						DGVFinal.Rows[num31].Cells[11].Value = tot_notinbahikhata_CESS;
						break;
					}
				}
			}
			int num32 = DGVFinal.Rows.Count - 1;
			for (int num33 = 0; num33 <= num32; num33++)
			{
				if (!Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[num33].Cells[0].Value, "Bills Found in both but mismatch", TextCompare: false))
				{
					continue;
				}
				int num34 = num33;
				int num35 = DGVFinal.Rows.Count - 1;
				for (int num36 = num34; num36 <= num35; num36++)
				{
					if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[num36].Cells[1].Value, "Total : ", TextCompare: false))
					{
						DGVFinal.Rows[num36].Cells[6].Value = tot_mismatch_Amt;
						DGVFinal.Rows[num36].Cells[7].Value = tot_mismatch_taxable;
						DGVFinal.Rows[num36].Cells[8].Value = tot_mismatch_IGST;
						DGVFinal.Rows[num36].Cells[9].Value = tot_mismatch_CGST;
						DGVFinal.Rows[num36].Cells[10].Value = tot_mismatch_SGST;
						DGVFinal.Rows[num36].Cells[11].Value = tot_mismatch_CESS;
						break;
					}
				}
			}
			DGVFinal.Columns[0].Visible = false;
			DGVFinal.DefaultCellStyle.Font = new Font("Verdana", 6f);
			int num37 = DGVFinal.ColumnCount - 1;
			for (int num38 = 0; num38 <= num37; num38++)
			{
				DGVFinal.Columns[num38].SortMode = (DataGridViewColumnSortMode)0;
			}
			LBL_Progress.Text = "";
			PB1.Value = 0;
			((Control)DGVFinal).Visible = true;
			((Control)PictureBox1).Visible = false;
			SET_Cols_width();
		}
	}

	private void SET_Cols_width()
	{
		DGVFinal.Columns[1].Width = 130;
		DGVFinal.Columns[2].Width = 85;
		DGVFinal.Columns[3].Width = 72;
		DGVFinal.Columns[4].Width = 55;
		DGVFinal.Columns[5].Width = 55;
		DGVFinal.Columns[6].Width = 55;
		DGVFinal.Columns[7].Width = 55;
		DGVFinal.Columns[8].Width = 55;
		DGVFinal.Columns[9].Width = 55;
		DGVFinal.Columns[10].Width = 55;
		DGVFinal.Columns[11].Width = 55;
		DGVFinal.Columns[12].Width = 55;
	}

	private void FINAL_OUTPUT()
	{
		//IL_004d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0053: Invalid comparison between Unknown and I4
		//IL_002a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0dbc: Unknown result type (might be due to invalid IL or missing references)
		Application application = (Application)Activator.CreateInstance(Marshal.GetTypeFromCLSID(new Guid("00024500-0000-0000-C000-000000000046")));
		if (application == null)
		{
			MessageBox.Show("Excel is not properly installed!!");
			return;
		}
		((FileDialog)SaveFileDialog1).Filter = "EXCEL Files (*.xlsx*)|*.xlsx";
		string fileName = default;
		if ((int)((CommonDialog)SaveFileDialog1).ShowDialog() == 1)
		{
			fileName = ((FileDialog)SaveFileDialog1).FileName;
		}
		object value = Missing.Value;
		Microsoft.Office.Interop.Excel.Workbook workbook = application.Workbooks.Add(RuntimeHelpers.GetObjectValue(value));
		Microsoft.Office.Interop.Excel.Worksheet worksheet = (Microsoft.Office.Interop.Excel.Worksheet)workbook.Sheets["sheet1"];
		DGVFinal.Sort(DGVFinal.Columns[0], ListSortDirection.Ascending);
		PB1.Value = 0;
		checked
		{
			PB1.Maximum = (int)Math.Round(Conversion.Val(DGVFinal.Rows.Count) + 3.0);
			bool flag = false;
			bool flag2 = false;
			bool flag3 = false;
			bool flag4 = false;
			int num = DGVFinal.Rows.Count - 1;
			for (int i = 0; i <= num; i++)
			{
				if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[i].Cells[0].Value, "Bills Matched", TextCompare: false))
				{
					if (!flag)
					{
						worksheet.Cells[i + 1, 1] = "Bills Matched";
						worksheet.Cells[i + 1, 2] = match;
					}
					flag = true;
					worksheet.Cells[i + 2, 3] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[1].Value);
					worksheet.Cells[i + 2, 4] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[2].Value);
					worksheet.Cells[i + 2, 5] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[3].Value);
					worksheet.Cells[i + 2, 6] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[4].Value);
					worksheet.Cells[i + 2, 7] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[5].Value);
					worksheet.Cells[i + 2, 8] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[6].Value);
					worksheet.Cells[i + 2, 9] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[7].Value);
					worksheet.Cells[i + 2, 10] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[8].Value);
					worksheet.Cells[i + 2, 11] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[9].Value);
				}
				if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[i].Cells[0].Value, "Bills Found in both but mismatch", TextCompare: false))
				{
					if (!flag2)
					{
						worksheet.Cells[i + 1, 1] = "Bills Found in both but mismatch";
						worksheet.Cells[i + 1, 2] = mismatch;
					}
					flag2 = true;
					worksheet.Cells[i + 2, 3] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[1].Value);
					worksheet.Cells[i + 2, 4] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[2].Value);
					worksheet.Cells[i + 2, 5] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[3].Value);
					worksheet.Cells[i + 2, 6] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[4].Value);
					worksheet.Cells[i + 2, 7] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[5].Value);
					worksheet.Cells[i + 2, 8] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[6].Value);
					worksheet.Cells[i + 2, 9] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[7].Value);
					worksheet.Cells[i + 2, 10] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[8].Value);
					worksheet.Cells[i + 2, 11] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[9].Value);
				}
				if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[i].Cells[0].Value, "Bills Present at Portal but not found in bahi-khata", TextCompare: false))
				{
					if (!flag3)
					{
						worksheet.Cells[i + 1, 1] = "Bills Present at Portal but not found in bahi-khata";
						worksheet.Cells[i + 1, 2] = notinbahikhata;
					}
					flag3 = true;
					worksheet.Cells[i + 2, 3] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[1].Value);
					worksheet.Cells[i + 2, 4] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[2].Value);
					worksheet.Cells[i + 2, 5] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[3].Value);
					worksheet.Cells[i + 2, 6] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[4].Value);
					worksheet.Cells[i + 2, 7] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[5].Value);
					worksheet.Cells[i + 2, 8] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[6].Value);
					worksheet.Cells[i + 2, 9] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[7].Value);
					worksheet.Cells[i + 2, 10] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[8].Value);
					worksheet.Cells[i + 2, 11] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[9].Value);
				}
				if (Operators.ConditionalCompareObjectEqual(DGVFinal.Rows[i].Cells[0].Value, "Bills in Bahi-Khata But Not Present at Portal", TextCompare: false))
				{
					if (!flag4)
					{
						worksheet.Cells[i + 1, 1] = "Bills in Bahi-Khata But Not Present at Portal";
						worksheet.Cells[i + 1, 2] = notinportal;
					}
					flag4 = true;
					worksheet.Cells[i + 2, 3] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[1].Value);
					worksheet.Cells[i + 2, 4] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[2].Value);
					worksheet.Cells[i + 2, 5] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[3].Value);
					worksheet.Cells[i + 2, 6] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[4].Value);
					worksheet.Cells[i + 2, 7] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[5].Value);
					worksheet.Cells[i + 2, 8] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[6].Value);
					worksheet.Cells[i + 2, 9] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[7].Value);
					worksheet.Cells[i + 2, 10] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[8].Value);
					worksheet.Cells[i + 2, 11] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[9].Value);
				}
				PB1.Value = (int)Math.Round(Conversion.Val(PB1.Value) + 1.0);
				LBL_Progress.Text = "Creating Excel file... " + Conversions.ToString(PB1.Value) + " %";
				Thread.Sleep(25);
			}
			workbook.SaveAs(fileName, XlFileFormat.xlWorkbookNormal, RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), XlSaveAsAccessMode.xlExclusive, RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value));
			workbook.Close(true, RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value));
			application.Quit();
			releaseObject(worksheet);
			releaseObject(workbook);
			releaseObject(application);
			MessageBox.Show("Excel file created , you can find the file " + fileName);
		}
	}

	private void btnExcel_Click(object sender, EventArgs e)
	{
		//IL_003c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0042: Invalid comparison between Unknown and I4
		//IL_002a: Unknown result type (might be due to invalid IL or missing references)
		Application application = (Application)Activator.CreateInstance(Marshal.GetTypeFromCLSID(new Guid("00024500-0000-0000-C000-000000000046")));
		if (application == null)
		{
			MessageBox.Show("Excel is not properly installed!!");
		}
		else
		{
			if ((int)((CommonDialog)FolderBrowserDialog1).ShowDialog() != 1)
			{
				return;
			}
			string text = FolderBrowserDialog1.SelectedPath + "\\" + file_nm + ".xls";
			object value = Missing.Value;
			Microsoft.Office.Interop.Excel.Workbook workbook = application.Workbooks.Add(RuntimeHelpers.GetObjectValue(value));
			Microsoft.Office.Interop.Excel.Worksheet worksheet = (Microsoft.Office.Interop.Excel.Worksheet)workbook.Sheets["Sheet1"];
			((Control)this).Cursor = Module1.CreateCursor(cursor_file);
			PB1.Value = 0;
			((Control)PB1).Visible = true;
			checked
			{
				PB1.Maximum = (int)Math.Round(Conversion.Val(DGVFinal.Rows.Count) + Conversion.Val(DGVPortal_Data.Rows.Count) + Conversion.Val(DGVBahikhata_data.Rows.Count));
				int num = DGVFinal.Rows.Count - 1;
				for (int i = 0; i <= num; i++)
				{
					worksheet.Cells[i + 1, 1] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[1].Value);
					worksheet.Cells[i + 1, 2] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[2].Value);
					worksheet.Cells[i + 1, 3] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[3].Value);
					worksheet.Cells[i + 1, 4] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[4].Value);
					worksheet.Cells[i + 1, 5] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[5].Value);
					worksheet.Cells[i + 1, 6] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[6].Value);
					worksheet.Cells[i + 1, 7] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[7].Value);
					worksheet.Cells[i + 1, 8] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[8].Value);
					worksheet.Cells[i + 1, 9] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[9].Value);
					worksheet.Cells[i + 1, 10] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[10].Value);
					worksheet.Cells[i + 1, 11] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[11].Value);
					worksheet.Cells[i + 1, 12] = RuntimeHelpers.GetObjectValue(DGVFinal.Rows[i].Cells[12].Value);
					Application.DoEvents();
					PB1.Value = (int)Math.Round(Conversion.Val(PB1.Value) + 1.0);
					LBL_Progress.Text = "Transferring Data to Excel";
					Thread.Sleep(25);
				}
				worksheet = (Microsoft.Office.Interop.Excel.Worksheet)workbook.Sheets["Sheet2"];
				worksheet.Cells[1, 1] = "Bills Import From Portal";
				worksheet.Cells[2, 1] = "GSTIN";
				worksheet.Cells[2, 2] = "Party Name";
				worksheet.Cells[2, 3] = "Supplier Return File Date";
				worksheet.Cells[2, 4] = "Period";
				worksheet.Cells[2, 5] = "Bill No.";
				worksheet.Cells[2, 6] = "Type";
				worksheet.Cells[2, 7] = "Bill Date";
				worksheet.Cells[2, 8] = "Bill Amt";
				worksheet.Cells[2, 9] = "Place of Supply\tRevCharge";
				worksheet.Cells[2, 10] = "rev";
				worksheet.Cells[2, 11] = "itcavl";
				worksheet.Cells[2, 12] = "rsn";
				worksheet.Cells[2, 13] = "diffprcnt";
				worksheet.Cells[2, 14] = "num";
				worksheet.Cells[2, 15] = "rt";
				worksheet.Cells[2, 16] = "IGST";
				worksheet.Cells[2, 17] = "CGST";
				worksheet.Cells[2, 18] = "SGST";
				worksheet.Cells[2, 19] = "CESS";
				int num2 = DGVPortal_Data.Rows.Count - 1;
				for (int j = 0; j <= num2; j++)
				{
					int num3 = ((BaseCollection)DGVPortal_Data.Columns).Count - 1;
					for (int k = 0; k <= num3; k++)
					{
						worksheet.Cells[j + 3, k + 1] = RuntimeHelpers.GetObjectValue(DGVPortal_Data.Rows[j].Cells[k].Value);
					}
					Application.DoEvents();
					PB1.Value = (int)Math.Round(Conversion.Val(PB1.Value) + 1.0);
					LBL_Progress.Text = "Transferring Data to Excel";
					Thread.Sleep(25);
				}
				worksheet = (Microsoft.Office.Interop.Excel.Worksheet)workbook.Sheets["Sheet3"];
				worksheet.Cells[1, 1] = "Bills Import From Bahi-Khata";
				worksheet.Cells[2, 1] = "tmpID";
				worksheet.Cells[2, 2] = "GSTIN";
				worksheet.Cells[2, 3] = "Party Name";
				worksheet.Cells[2, 4] = "Transaction";
				worksheet.Cells[2, 5] = "Bill Type";
				worksheet.Cells[2, 6] = "Bill No.";
				worksheet.Cells[2, 7] = "Bill Date";
				worksheet.Cells[2, 8] = "Taxable Amt";
				worksheet.Cells[2, 9] = "IGST";
				worksheet.Cells[2, 10] = "CGST";
				worksheet.Cells[2, 11] = "SGST";
				worksheet.Cells[2, 12] = "CESS";
				int num4 = DGVBahikhata_data.Rows.Count - 1;
				for (int l = 0; l <= num4; l++)
				{
					int num5 = ((BaseCollection)DGVBahikhata_data.Columns).Count - 1;
					for (int m = 0; m <= num5; m++)
					{
						worksheet.Cells[l + 3, m + 3] = RuntimeHelpers.GetObjectValue(DGVBahikhata_data.Rows[l].Cells[m].Value);
					}
					Application.DoEvents();
					PB1.Value = (int)Math.Round(Conversion.Val(PB1.Value) + 1.0);
					LBL_Progress.Text = "Transferring Data to Excel";
					Thread.Sleep(25);
				}
				PB1.Value = 0;
				((Control)PB1).Visible = false;
				workbook.SaveAs(text, XlFileFormat.xlWorkbookNormal, RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), XlSaveAsAccessMode.xlExclusive, RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value));
				workbook.Close(true, RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value));
				application.Quit();
				releaseObject(worksheet);
				releaseObject(workbook);
				releaseObject(application);
				((Control)this).Cursor = Cursors.Default;
			}
			string value2 = Conversions.ToString((int)Interaction.MsgBox("Process Complete\r\n\r\nDo You Want to Open this File ?", MsgBoxStyle.YesNo | MsgBoxStyle.Information, "GSTR-2 Result"));
			if (Conversions.ToDouble(value2) == 6.0)
			{
				Process.Start(text);
			}
		}
	}

	private void Form1_KeyDown(object sender, KeyEventArgs e)
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0009: Invalid comparison between Unknown and I4
		if ((int)e.KeyCode == 116)
		{
			if (((Control)BtnGetOTP).Visible)
			{
				((Control)BtnGetOTP).Visible = false;
			}
			else
			{
				((Control)BtnGetOTP).Visible = true;
			}
			if (((Control)DGVPortal_Data).Visible)
			{
				((Control)DGVPortal_Data).Visible = false;
			}
			else
			{
				((Control)DGVPortal_Data).Visible = true;
			}
			if (((Control)DGVBahikhata_data).Visible)
			{
				((Control)DGVBahikhata_data).Visible = false;
			}
			else
			{
				((Control)DGVBahikhata_data).Visible = true;
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
		//IL_00a2: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ac: Expected Obj, but got Unknown
		//IL_00ae: Unknown result type (might be due to invalid IL or missing references)
		//IL_00b8: Expected Obj, but got Unknown
		//IL_00ba: Unknown result type (might be due to invalid IL or missing references)
		//IL_00c4: Expected Obj, but got Unknown
		//IL_00c6: Unknown result type (might be due to invalid IL or missing references)
		//IL_00d0: Expected Obj, but got Unknown
		//IL_00d2: Unknown result type (might be due to invalid IL or missing references)
		//IL_00dc: Expected Obj, but got Unknown
		//IL_00de: Unknown result type (might be due to invalid IL or missing references)
		//IL_00e8: Expected Obj, but got Unknown
		//IL_00ea: Unknown result type (might be due to invalid IL or missing references)
		//IL_00f4: Expected Obj, but got Unknown
		//IL_00f6: Unknown result type (might be due to invalid IL or missing references)
		//IL_0100: Expected Obj, but got Unknown
		//IL_0102: Unknown result type (might be due to invalid IL or missing references)
		//IL_010c: Expected Obj, but got Unknown
		//IL_010e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0118: Expected Obj, but got Unknown
		//IL_011a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0124: Expected Obj, but got Unknown
		//IL_0126: Unknown result type (might be due to invalid IL or missing references)
		//IL_0130: Expected Obj, but got Unknown
		//IL_0132: Unknown result type (might be due to invalid IL or missing references)
		//IL_013c: Expected Obj, but got Unknown
		//IL_013e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0148: Expected Obj, but got Unknown
		//IL_014a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0154: Expected Obj, but got Unknown
		//IL_0156: Unknown result type (might be due to invalid IL or missing references)
		//IL_0160: Expected Obj, but got Unknown
		//IL_0162: Unknown result type (might be due to invalid IL or missing references)
		//IL_016c: Expected Obj, but got Unknown
		//IL_016e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0178: Expected Obj, but got Unknown
		//IL_017a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0184: Expected Obj, but got Unknown
		//IL_0186: Unknown result type (might be due to invalid IL or missing references)
		//IL_0190: Expected Obj, but got Unknown
		//IL_0192: Unknown result type (might be due to invalid IL or missing references)
		//IL_019c: Expected Obj, but got Unknown
		//IL_019e: Unknown result type (might be due to invalid IL or missing references)
		//IL_01a8: Expected Obj, but got Unknown
		//IL_01aa: Unknown result type (might be due to invalid IL or missing references)
		//IL_01b4: Expected Obj, but got Unknown
		//IL_01b6: Unknown result type (might be due to invalid IL or missing references)
		//IL_01c0: Expected Obj, but got Unknown
		//IL_01c2: Unknown result type (might be due to invalid IL or missing references)
		//IL_01cc: Expected Obj, but got Unknown
		//IL_01ce: Unknown result type (might be due to invalid IL or missing references)
		//IL_01d8: Expected Obj, but got Unknown
		//IL_01da: Unknown result type (might be due to invalid IL or missing references)
		//IL_01e4: Expected Obj, but got Unknown
		//IL_02e9: Unknown result type (might be due to invalid IL or missing references)
		//IL_02f3: Expected Obj, but got Unknown
		//IL_037a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0384: Expected Obj, but got Unknown
		//IL_0476: Unknown result type (might be due to invalid IL or missing references)
		//IL_0480: Expected Obj, but got Unknown
		//IL_0507: Unknown result type (might be due to invalid IL or missing references)
		//IL_0511: Expected Obj, but got Unknown
		//IL_069b: Unknown result type (might be due to invalid IL or missing references)
		//IL_06a5: Expected Obj, but got Unknown
		//IL_0749: Unknown result type (might be due to invalid IL or missing references)
		//IL_0753: Expected Obj, but got Unknown
		//IL_07e4: Unknown result type (might be due to invalid IL or missing references)
		//IL_07ee: Expected Obj, but got Unknown
		//IL_0966: Unknown result type (might be due to invalid IL or missing references)
		//IL_0970: Expected Obj, but got Unknown
		//IL_0a0d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0a17: Expected Obj, but got Unknown
		//IL_0abc: Unknown result type (might be due to invalid IL or missing references)
		//IL_0ac6: Expected Obj, but got Unknown
		//IL_0b6b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0b75: Expected Obj, but got Unknown
		//IL_0c07: Unknown result type (might be due to invalid IL or missing references)
		//IL_0c11: Expected Obj, but got Unknown
		//IL_0ca6: Unknown result type (might be due to invalid IL or missing references)
		//IL_0cb0: Expected Obj, but got Unknown
		//IL_0d1a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0d24: Expected Obj, but got Unknown
		//IL_0dba: Unknown result type (might be due to invalid IL or missing references)
		//IL_0dc4: Expected Obj, but got Unknown
		//IL_0e3f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0e49: Expected Obj, but got Unknown
		//IL_0ee2: Unknown result type (might be due to invalid IL or missing references)
		//IL_0eec: Expected Obj, but got Unknown
		//IL_1079: Unknown result type (might be due to invalid IL or missing references)
		//IL_1083: Expected Obj, but got Unknown
		//IL_110c: Unknown result type (might be due to invalid IL or missing references)
		//IL_1116: Expected Obj, but got Unknown
		//IL_1195: Unknown result type (might be due to invalid IL or missing references)
		//IL_119f: Expected Obj, but got Unknown
		//IL_120d: Unknown result type (might be due to invalid IL or missing references)
		//IL_1217: Expected Obj, but got Unknown
		//IL_140e: Unknown result type (might be due to invalid IL or missing references)
		//IL_1523: Unknown result type (might be due to invalid IL or missing references)
		//IL_15b7: Unknown result type (might be due to invalid IL or missing references)
		//IL_1618: Unknown result type (might be due to invalid IL or missing references)
		//IL_1622: Expected Obj, but got Unknown
		//IL_16bb: Unknown result type (might be due to invalid IL or missing references)
		//IL_16c5: Expected Obj, but got Unknown
		//IL_1932: Unknown result type (might be due to invalid IL or missing references)
		//IL_193c: Expected Obj, but got Unknown
		ComponentResourceManager componentResourceManager = new ComponentResourceManager(typeof(Form1));
		Button1 = new Button();
		BTN_GEN_Token = new Button();
		BtnDownloadData = new Button();
		TextBox1 = new TextBox();
		btnJsonToExcel = new Button();
		btnExcelToAccess = new Button();
		PB1 = new ProgressBar();
		DGVdata = new DataGridView();
		CHKDownloadAlways = new CheckBox();
		RBAuto = new RadioButton();
		RBManual = new RadioButton();
		GroupBox1 = new GroupBox();
		Button5 = new Button();
		Button3 = new Button();
		btnExcel = new Button();
		Button2 = new Button();
		txtPath = new TextBox();
		BtnGetOTP = new Button();
		LBL_Progress = new Label();
		BtnVerifyOTP = new Button();
		OpenFileDialog1 = new OpenFileDialog();
		BTNCompare = new Button();
		DGVBahikhata_data = new DataGridView();
		DGVPortal_Data = new DataGridView();
		txtBillno = new TextBox();
		Label1 = new Label();
		Label2 = new Label();
		txtAmount = new TextBox();
		DGVFinal = new DataGridView();
		TabControl1 = new TabControl();
		TabPage1 = new TabPage();
		PictureBox1 = new PictureBox();
		TabPage2 = new TabPage();
		TabPage3 = new TabPage();
		SaveFileDialog1 = new SaveFileDialog();
		Button4 = new Button();
		LblPeriod = new Label();
		FolderBrowserDialog1 = new FolderBrowserDialog();
		Button6 = new Button();
		((ISupportInitialize)DGVdata).BeginInit();
		((Control)GroupBox1).SuspendLayout();
		((ISupportInitialize)DGVBahikhata_data).BeginInit();
		((ISupportInitialize)DGVPortal_Data).BeginInit();
		((ISupportInitialize)DGVFinal).BeginInit();
		((Control)TabControl1).SuspendLayout();
		((Control)TabPage1).SuspendLayout();
		((ISupportInitialize)PictureBox1).BeginInit();
		((Control)TabPage2).SuspendLayout();
		((Control)TabPage3).SuspendLayout();
		((Control)this).SuspendLayout();
		((Control)Button1).Location = new Point(1668, 8);
		((Control)Button1).Name = "Button1";
		((Control)Button1).Size = new Size(125, 50);
		((Control)Button1).TabIndex = 0;
		((ButtonBase)Button1).Text = "Button1";
		((ButtonBase)Button1).UseVisualStyleBackColor = true;
		((Control)Button1).Visible = false;
		((Control)BTN_GEN_Token).Font = new Font("Verdana", 8.5f);
		((Control)BTN_GEN_Token).Location = new Point(878, 121);
		((Control)BTN_GEN_Token).Name = "BTN_GEN_Token";
		((Control)BTN_GEN_Token).Size = new Size(91, 37);
		((Control)BTN_GEN_Token).TabIndex = 1;
		((ButtonBase)BTN_GEN_Token).Text = "Generate Token";
		((ButtonBase)BTN_GEN_Token).UseVisualStyleBackColor = true;
		((Control)BTN_GEN_Token).Visible = false;
		((Control)BtnDownloadData).Font = new Font("Verdana", 8.5f);
		((Control)BtnDownloadData).Location = new Point(706, 56);
		((Control)BtnDownloadData).Name = "BtnDownloadData";
		((Control)BtnDownloadData).Size = new Size(91, 36);
		((Control)BtnDownloadData).TabIndex = 2;
		((ButtonBase)BtnDownloadData).Text = "Download Data";
		((ButtonBase)BtnDownloadData).UseVisualStyleBackColor = true;
		((Control)BtnDownloadData).Visible = false;
		((Control)TextBox1).Location = new Point(12, 206);
		TextBox1.Multiline = true;
		((Control)TextBox1).Name = "TextBox1";
		((Control)TextBox1).Size = new Size(1775, 599);
		((Control)TextBox1).TabIndex = 3;
		((Control)TextBox1).Visible = false;
		((Control)btnJsonToExcel).Font = new Font("Verdana", 8.5f);
		((Control)btnJsonToExcel).Location = new Point(707, 92);
		((Control)btnJsonToExcel).Name = "btnJsonToExcel";
		((Control)btnJsonToExcel).Size = new Size(90, 36);
		((Control)btnJsonToExcel).TabIndex = 4;
		((ButtonBase)btnJsonToExcel).Text = "Json to Excel";
		((ButtonBase)btnJsonToExcel).UseVisualStyleBackColor = true;
		((Control)btnJsonToExcel).Visible = false;
		((Control)btnExcelToAccess).Font = new Font("Verdana", 8.5f);
		((Control)btnExcelToAccess).Location = new Point(799, 14);
		((Control)btnExcelToAccess).Name = "btnExcelToAccess";
		((Control)btnExcelToAccess).Size = new Size(80, 36);
		((Control)btnExcelToAccess).TabIndex = 5;
		((ButtonBase)btnExcelToAccess).Text = "Excel to Access";
		((ButtonBase)btnExcelToAccess).UseVisualStyleBackColor = true;
		((Control)btnExcelToAccess).Visible = false;
		((Control)PB1).Location = new Point(7, 89);
		((Control)PB1).Name = "PB1";
		((Control)PB1).Size = new Size(523, 23);
		PB1.Style = (ProgressBarStyle)1;
		((Control)PB1).TabIndex = 6;
		DGVdata.AllowUserToAddRows = false;
		DGVdata.AllowUserToDeleteRows = false;
		DGVdata.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
		((Control)DGVdata).Location = new Point(5, 159);
		((Control)DGVdata).Name = "DGVdata";
		DGVdata.ReadOnly = true;
		DGVdata.RowHeadersWidth = 51;
		((Control)DGVdata).Size = new Size(1645, 549);
		((Control)DGVdata).TabIndex = 7;
		((Control)DGVdata).Visible = false;
		((ButtonBase)CHKDownloadAlways).AutoSize = true;
		((Control)CHKDownloadAlways).Font = new Font("Verdana", 11.25f);
		((Control)CHKDownloadAlways).Location = new Point(275, 27);
		((Control)CHKDownloadAlways).Name = "CHKDownloadAlways";
		((Control)CHKDownloadAlways).Size = new Size(158, 22);
		((Control)CHKDownloadAlways).TabIndex = 8;
		((ButtonBase)CHKDownloadAlways).Text = "Download Always";
		((ButtonBase)CHKDownloadAlways).UseVisualStyleBackColor = true;
		((Control)CHKDownloadAlways).Visible = false;
		((ButtonBase)RBAuto).AutoSize = true;
		RBAuto.Checked = true;
		((Control)RBAuto).Font = new Font("Verdana", 11.25f);
		((Control)RBAuto).Location = new Point(7, 26);
		((Control)RBAuto).Name = "RBAuto";
		((Control)RBAuto).Size = new Size(102, 22);
		((Control)RBAuto).TabIndex = 10;
		RBAuto.TabStop = true;
		((ButtonBase)RBAuto).Text = "Automatic";
		((ButtonBase)RBAuto).UseVisualStyleBackColor = true;
		((ButtonBase)RBManual).AutoSize = true;
		((Control)RBManual).Font = new Font("Verdana", 11.25f);
		((Control)RBManual).Location = new Point(161, 26);
		((Control)RBManual).Name = "RBManual";
		((Control)RBManual).Size = new Size(90, 22);
		((Control)RBManual).TabIndex = 11;
		((ButtonBase)RBManual).Text = "Manually";
		((ButtonBase)RBManual).UseVisualStyleBackColor = true;
		((Control)GroupBox1).Controls.Add((Control)(object)Button5);
		((Control)GroupBox1).Controls.Add((Control)(object)Button3);
		((Control)GroupBox1).Controls.Add((Control)(object)btnExcel);
		((Control)GroupBox1).Controls.Add((Control)(object)Button2);
		((Control)GroupBox1).Controls.Add((Control)(object)txtPath);
		((Control)GroupBox1).Controls.Add((Control)(object)RBManual);
		((Control)GroupBox1).Controls.Add((Control)(object)CHKDownloadAlways);
		((Control)GroupBox1).Controls.Add((Control)(object)RBAuto);
		((Control)GroupBox1).Controls.Add((Control)(object)PB1);
		((Control)GroupBox1).Controls.Add((Control)(object)BtnGetOTP);
		((Control)GroupBox1).Controls.Add((Control)(object)LBL_Progress);
		((Control)GroupBox1).Font = new Font("Verdana", 11.25f);
		((Control)GroupBox1).Location = new Point(3, 5);
		((Control)GroupBox1).Name = "GroupBox1";
		((Control)GroupBox1).Size = new Size(534, 142);
		((Control)GroupBox1).TabIndex = 12;
		GroupBox1.TabStop = false;
		GroupBox1.Text = "Choose GSTR-2 Compare Method";
		((ButtonBase)Button5).BackColor = Color.FromArgb(255, 255, 128);
		((Control)Button5).Font = new Font("Verdana", 9f, (FontStyle)1);
		((Control)Button5).ForeColor = Color.Black;
		((Control)Button5).Location = new Point(449, 12);
		((Control)Button5).Name = "Button5";
		((Control)Button5).Size = new Size(81, 28);
		((Control)Button5).TabIndex = 28;
		((ButtonBase)Button5).Text = "&Exit";
		((ButtonBase)Button5).UseVisualStyleBackColor = false;
		((ButtonBase)Button3).BackColor = Color.FromArgb(192, 0, 0);
		((Control)Button3).Font = new Font("Verdana", 9f, (FontStyle)1);
		((Control)Button3).ForeColor = Color.Yellow;
		((Control)Button3).Location = new Point(275, 113);
		((Control)Button3).Name = "Button3";
		((Control)Button3).Size = new Size(121, 25);
		((Control)Button3).TabIndex = 17;
		((ButtonBase)Button3).Text = "Start Process";
		((ButtonBase)Button3).UseVisualStyleBackColor = false;
		((ButtonBase)btnExcel).BackColor = Color.FromArgb(192, 0, 0);
		((Control)btnExcel).Font = new Font("Verdana", 9f, (FontStyle)1);
		((Control)btnExcel).ForeColor = Color.Yellow;
		((Control)btnExcel).Location = new Point(395, 113);
		((Control)btnExcel).Name = "btnExcel";
		((Control)btnExcel).Size = new Size(133, 25);
		((Control)btnExcel).TabIndex = 25;
		((ButtonBase)btnExcel).Text = "Transfer To Excel";
		((ButtonBase)btnExcel).UseVisualStyleBackColor = false;
		((Control)Button2).Font = new Font("Microsoft Sans Serif", 8f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
		((Control)Button2).Location = new Point(406, 61);
		((Control)Button2).Name = "Button2";
		((Control)Button2).Size = new Size(121, 23);
		((Control)Button2).TabIndex = 16;
		((ButtonBase)Button2).Text = "Choose JSON File";
		((ButtonBase)Button2).TextAlign = (ContentAlignment)2;
		((ButtonBase)Button2).UseVisualStyleBackColor = true;
		((Control)Button2).Visible = false;
		((Control)txtPath).Font = new Font("Verdana", 10f);
		((Control)txtPath).Location = new Point(5, 59);
		((Control)txtPath).Name = "txtPath";
		((Control)txtPath).Size = new Size(397, 24);
		((Control)txtPath).TabIndex = 16;
		((Control)txtPath).Visible = false;
		((Control)BtnGetOTP).Font = new Font("Verdana", 8f, (FontStyle)1);
		((Control)BtnGetOTP).Location = new Point(450, 39);
		((Control)BtnGetOTP).Name = "BtnGetOTP";
		((Control)BtnGetOTP).Size = new Size(79, 23);
		((Control)BtnGetOTP).TabIndex = 13;
		((ButtonBase)BtnGetOTP).Text = "Get OTP";
		((ButtonBase)BtnGetOTP).UseVisualStyleBackColor = true;
		((Control)BtnGetOTP).Visible = false;
		LBL_Progress.AutoSize = true;
		((Control)LBL_Progress).Font = new Font("Verdana", 10f, (FontStyle)1);
		((Control)LBL_Progress).ForeColor = Color.Green;
		((Control)LBL_Progress).Location = new Point(6, 116);
		((Control)LBL_Progress).Name = "LBL_Progress";
		((Control)LBL_Progress).Size = new Size(59, 17);
		((Control)LBL_Progress).TabIndex = 15;
		LBL_Progress.Text = "Label1";
		((Control)BtnVerifyOTP).Font = new Font("Verdana", 8.5f);
		((Control)BtnVerifyOTP).Location = new Point(707, 33);
		((Control)BtnVerifyOTP).Name = "BtnVerifyOTP";
		((Control)BtnVerifyOTP).Size = new Size(90, 24);
		((Control)BtnVerifyOTP).TabIndex = 14;
		((ButtonBase)BtnVerifyOTP).Text = "Verify OTP";
		((ButtonBase)BtnVerifyOTP).UseVisualStyleBackColor = true;
		((Control)BtnVerifyOTP).Visible = false;
		((FileDialog)OpenFileDialog1).FileName = "OpenFileDialog1";
		((Control)BTNCompare).Font = new Font("Verdana", 8.5f);
		((Control)BTNCompare).Location = new Point(799, 50);
		((Control)BTNCompare).Name = "BTNCompare";
		((Control)BTNCompare).Size = new Size(80, 32);
		((Control)BTNCompare).TabIndex = 16;
		((ButtonBase)BTNCompare).Text = "Compare";
		((ButtonBase)BTNCompare).UseVisualStyleBackColor = true;
		((Control)BTNCompare).Visible = false;
		DGVBahikhata_data.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
		((Control)DGVBahikhata_data).Location = new Point(6, 6);
		((Control)DGVBahikhata_data).Name = "DGVBahikhata_data";
		DGVBahikhata_data.ReadOnly = true;
		DGVBahikhata_data.RowHeadersVisible = false;
		DGVBahikhata_data.RowHeadersWidth = 51;
		((Control)DGVBahikhata_data).Size = new Size(1711, 585);
		((Control)DGVBahikhata_data).TabIndex = 17;
		DGVPortal_Data.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
		((Control)DGVPortal_Data).Location = new Point(10, 6);
		((Control)DGVPortal_Data).Name = "DGVPortal_Data";
		DGVPortal_Data.ReadOnly = true;
		DGVPortal_Data.RowHeadersVisible = false;
		DGVPortal_Data.RowHeadersWidth = 51;
		((Control)DGVPortal_Data).Size = new Size(1702, 585);
		((Control)DGVPortal_Data).TabIndex = 18;
		((Control)txtBillno).Font = new Font("Verdana", 8.5f);
		((Control)txtBillno).Location = new Point(545, 87);
		((Control)txtBillno).Name = "txtBillno";
		((Control)txtBillno).Size = new Size(79, 21);
		((Control)txtBillno).TabIndex = 19;
		txtBillno.Text = "0";
		txtBillno.TextAlign = (HorizontalAlignment)1;
		Label1.AutoSize = true;
		((Control)Label1).Font = new Font("Verdana", 8.5f, (FontStyle)4);
		((Control)Label1).Location = new Point(544, 68);
		((Control)Label1).Name = "Label1";
		((Control)Label1).Size = new Size(161, 14);
		((Control)Label1).TabIndex = 20;
		Label1.Text = "Count Char Right Bill No.";
		Label2.AutoSize = true;
		((Control)Label2).Font = new Font("Verdana", 8.5f, (FontStyle)4);
		((Control)Label2).Location = new Point(543, 14);
		((Control)Label2).Name = "Label2";
		((Control)Label2).Size = new Size(110, 14);
		((Control)Label2).TabIndex = 22;
		Label2.Text = "Ignore Amount :";
		((Control)txtAmount).Font = new Font("Verdana", 8.5f);
		((Control)txtAmount).Location = new Point(546, 32);
		((Control)txtAmount).Name = "txtAmount";
		((Control)txtAmount).Size = new Size(79, 21);
		((Control)txtAmount).TabIndex = 21;
		txtAmount.Text = "1.00";
		txtAmount.TextAlign = (HorizontalAlignment)1;
		DGVFinal.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
		((Control)DGVFinal).Location = new Point(2, 6);
		((Control)DGVFinal).Name = "DGVFinal";
		DGVFinal.ReadOnly = true;
		DGVFinal.RowHeadersVisible = false;
		DGVFinal.RowHeadersWidth = 51;
		((Control)DGVFinal).Size = new Size(913, 281);
		((Control)DGVFinal).TabIndex = 23;
		((Control)DGVFinal).Visible = false;
		((Control)TabControl1).Controls.Add((Control)(object)TabPage1);
		((Control)TabControl1).Controls.Add((Control)(object)TabPage2);
		((Control)TabControl1).Controls.Add((Control)(object)TabPage3);
		((Control)TabControl1).Location = new Point(2, 162);
		((Control)TabControl1).Name = "TabControl1";
		TabControl1.SelectedIndex = 0;
		((Control)TabControl1).Size = new Size(929, 319);
		((Control)TabControl1).TabIndex = 24;
		((Control)TabPage1).Controls.Add((Control)(object)PictureBox1);
		((Control)TabPage1).Controls.Add((Control)(object)DGVFinal);
		TabPage1.Location = new Point(4, 22);
		((Control)TabPage1).Name = "TabPage1";
		((Control)TabPage1).Padding = new Padding(3);
		((Control)TabPage1).Size = new Size(921, 293);
		TabPage1.TabIndex = 0;
		TabPage1.Text = "RESULT";
		TabPage1.UseVisualStyleBackColor = true;
		((Control)PictureBox1).Dock = (DockStyle)3;
		((Control)PictureBox1).Location = new Point(3, 3);
		((Control)PictureBox1).Name = "PictureBox1";
		((Control)PictureBox1).Size = new Size(350, 287);
		PictureBox1.SizeMode = (PictureBoxSizeMode)3;
		PictureBox1.TabIndex = 24;
		PictureBox1.TabStop = false;
		((Control)PictureBox1).Visible = false;
		((Control)TabPage2).Controls.Add((Control)(object)DGVPortal_Data);
		TabPage2.Location = new Point(4, 22);
		((Control)TabPage2).Name = "TabPage2";
		((Control)TabPage2).Padding = new Padding(3);
		((Control)TabPage2).Size = new Size(921, 293);
		TabPage2.TabIndex = 1;
		TabPage2.Text = "-";
		TabPage2.UseVisualStyleBackColor = true;
		((Control)TabPage3).Controls.Add((Control)(object)DGVBahikhata_data);
		TabPage3.Location = new Point(4, 22);
		((Control)TabPage3).Name = "TabPage3";
		((Control)TabPage3).Padding = new Padding(3);
		((Control)TabPage3).Size = new Size(921, 293);
		TabPage3.TabIndex = 2;
		TabPage3.Text = "-";
		TabPage3.UseVisualStyleBackColor = true;
		((Control)Button4).Font = new Font("Verdana", 8.5f);
		((Control)Button4).Location = new Point(695, 134);
		((Control)Button4).Name = "Button4";
		((Control)Button4).Size = new Size(59, 22);
		((Control)Button4).TabIndex = 26;
		((ButtonBase)Button4).Text = "TEST";
		((ButtonBase)Button4).UseVisualStyleBackColor = true;
		((Control)Button4).Visible = false;
		LblPeriod.AutoSize = true;
		((Control)LblPeriod).Font = new Font("Verdana", 10f, (FontStyle)1);
		((Control)LblPeriod).ForeColor = Color.Green;
		((Control)LblPeriod).Location = new Point(216, 164);
		((Control)LblPeriod).Name = "LblPeriod";
		((Control)LblPeriod).Size = new Size(59, 17);
		((Control)LblPeriod).TabIndex = 27;
		LblPeriod.Text = "Label1";
		((Control)Button6).Location = new Point(958, 34);
		((Control)Button6).Name = "Button6";
		((Control)Button6).Size = new Size(98, 57);
		((Control)Button6).TabIndex = 28;
		((ButtonBase)Button6).Text = "EXTRA";
		((ButtonBase)Button6).UseVisualStyleBackColor = true;
		((Control)Button6).Visible = false;
		((ContainerControl)this).AutoScaleDimensions = new SizeF(6f, 13f);
		((ContainerControl)this).AutoScaleMode = (AutoScaleMode)1;
		((Form)this).ClientSize = new Size(1216, 720);
		((Control)this).Controls.Add((Control)(object)Button6);
		((Control)this).Controls.Add((Control)(object)LblPeriod);
		((Control)this).Controls.Add((Control)(object)Button4);
		((Control)this).Controls.Add((Control)(object)TabControl1);
		((Control)this).Controls.Add((Control)(object)Label2);
		((Control)this).Controls.Add((Control)(object)txtAmount);
		((Control)this).Controls.Add((Control)(object)Label1);
		((Control)this).Controls.Add((Control)(object)txtBillno);
		((Control)this).Controls.Add((Control)(object)BTNCompare);
		((Control)this).Controls.Add((Control)(object)BtnVerifyOTP);
		((Control)this).Controls.Add((Control)(object)GroupBox1);
		((Control)this).Controls.Add((Control)(object)btnExcelToAccess);
		((Control)this).Controls.Add((Control)(object)btnJsonToExcel);
		((Control)this).Controls.Add((Control)(object)TextBox1);
		((Control)this).Controls.Add((Control)(object)BtnDownloadData);
		((Control)this).Controls.Add((Control)(object)BTN_GEN_Token);
		((Control)this).Controls.Add((Control)(object)Button1);
		((Control)this).Controls.Add((Control)(object)DGVdata);
		((Form)this).Icon = (Icon)componentResourceManager.GetObject("$this.Icon");
		((Control)this).Name = "Form1";
		((Form)this).Text = "Bahi-Khata Automatic GSTR-2 Matching Tool";
		((Form)this).WindowState = (FormWindowState)2;
		((ISupportInitialize)DGVdata).EndInit();
		((Control)GroupBox1).ResumeLayout(false);
		((Control)GroupBox1).PerformLayout();
		((ISupportInitialize)DGVBahikhata_data).EndInit();
		((ISupportInitialize)DGVPortal_Data).EndInit();
		((ISupportInitialize)DGVFinal).EndInit();
		((Control)TabControl1).ResumeLayout(false);
		((Control)TabPage1).ResumeLayout(false);
		((ISupportInitialize)PictureBox1).EndInit();
		((Control)TabPage2).ResumeLayout(false);
		((Control)TabPage3).ResumeLayout(false);
		((Control)this).ResumeLayout(false);
		((Control)this).PerformLayout();
	}
}
