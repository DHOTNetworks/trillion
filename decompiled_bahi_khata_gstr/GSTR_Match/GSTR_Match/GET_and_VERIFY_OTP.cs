using System;
using System.ComponentModel;
using System.Data.OleDb;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Net;
using System.Runtime.CompilerServices;
using System.Text;
using System.Windows.Forms;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.CompilerServices;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace GSTR_Match;

[DesignerGenerated]
public class GET_and_VERIFY_OTP : Form
{
	private IContainer components;

	[field: AccessedThroughProperty("LBL_OTP")]
	internal virtual Label LBL_OTP
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

	[field: AccessedThroughProperty("txtOTP")]
	internal virtual TextBox txtOTP
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

	[field: AccessedThroughProperty("btnExit")]
	internal virtual Button btnExit
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		[CompilerGenerated]
		set
		{
			EventHandler eventHandler = btnExit_Click;
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

	public GET_and_VERIFY_OTP()
	{
		((Form)this).Load += GET_and_VERIFY_OTP_Load;
		InitializeComponent();
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
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_000c: Expected Obj, but got Unknown
		//IL_000e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0018: Expected Obj, but got Unknown
		//IL_001a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0024: Expected Obj, but got Unknown
		//IL_0026: Unknown result type (might be due to invalid IL or missing references)
		//IL_0030: Expected Obj, but got Unknown
		//IL_0032: Unknown result type (might be due to invalid IL or missing references)
		//IL_003c: Expected Obj, but got Unknown
		//IL_003e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0048: Expected Obj, but got Unknown
		//IL_006e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0078: Expected Obj, but got Unknown
		//IL_00e1: Unknown result type (might be due to invalid IL or missing references)
		//IL_00eb: Expected Obj, but got Unknown
		//IL_0164: Unknown result type (might be due to invalid IL or missing references)
		//IL_016e: Expected Obj, but got Unknown
		//IL_01d9: Unknown result type (might be due to invalid IL or missing references)
		//IL_01e3: Expected Obj, but got Unknown
		//IL_024f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0259: Expected Obj, but got Unknown
		//IL_02d5: Unknown result type (might be due to invalid IL or missing references)
		//IL_02df: Expected Obj, but got Unknown
		LBL_OTP = new Label();
		BtnGetOTP = new Button();
		txtOTP = new TextBox();
		Label1 = new Label();
		BtnVerifyOTP = new Button();
		btnExit = new Button();
		((Control)this).SuspendLayout();
		LBL_OTP.AutoSize = true;
		((Control)LBL_OTP).Font = new Font("Verdana", 11.25f, (FontStyle)1);
		((Control)LBL_OTP).Location = new Point(7, 6);
		((Control)LBL_OTP).Name = "LBL_OTP";
		((Control)LBL_OTP).Size = new Size(63, 18);
		((Control)LBL_OTP).TabIndex = 0;
		LBL_OTP.Text = "Label1";
		((Control)BtnGetOTP).Font = new Font("Verdana", 11.25f, (FontStyle)1);
		((Control)BtnGetOTP).Location = new Point(10, 82);
		((Control)BtnGetOTP).Name = "BtnGetOTP";
		((Control)BtnGetOTP).Size = new Size(117, 43);
		((Control)BtnGetOTP).TabIndex = 14;
		((ButtonBase)BtnGetOTP).Text = "Get OTP";
		((ButtonBase)BtnGetOTP).UseVisualStyleBackColor = true;
		((Control)txtOTP).Font = new Font("Verdana", 11.25f, (FontStyle)1);
		((Control)txtOTP).Location = new Point(120, 41);
		((Control)txtOTP).Name = "txtOTP";
		((Control)txtOTP).Size = new Size(253, 26);
		((Control)txtOTP).TabIndex = 15;
		Label1.AutoSize = true;
		((Control)Label1).Font = new Font("Verdana", 11.25f, (FontStyle)1);
		((Control)Label1).Location = new Point(12, 44);
		((Control)Label1).Name = "Label1";
		((Control)Label1).Size = new Size(102, 18);
		((Control)Label1).TabIndex = 16;
		Label1.Text = "Enter OTP :";
		((Control)BtnVerifyOTP).Font = new Font("Verdana", 11.25f, (FontStyle)1);
		((Control)BtnVerifyOTP).Location = new Point(133, 82);
		((Control)BtnVerifyOTP).Name = "BtnVerifyOTP";
		((Control)BtnVerifyOTP).Size = new Size(117, 43);
		((Control)BtnVerifyOTP).TabIndex = 17;
		((ButtonBase)BtnVerifyOTP).Text = "Verify OTP";
		((ButtonBase)BtnVerifyOTP).UseVisualStyleBackColor = true;
		((Control)btnExit).Font = new Font("Verdana", 11.25f, (FontStyle)1);
		((Control)btnExit).Location = new Point(256, 82);
		((Control)btnExit).Name = "btnExit";
		((Control)btnExit).Size = new Size(117, 43);
		((Control)btnExit).TabIndex = 18;
		((ButtonBase)btnExit).Text = "&Exit";
		((ButtonBase)btnExit).UseVisualStyleBackColor = true;
		((ContainerControl)this).AutoScaleDimensions = new SizeF(6f, 13f);
		((ContainerControl)this).AutoScaleMode = (AutoScaleMode)1;
		((Form)this).ClientSize = new Size(391, 138);
		((Control)this).Controls.Add((Control)(object)btnExit);
		((Control)this).Controls.Add((Control)(object)BtnVerifyOTP);
		((Control)this).Controls.Add((Control)(object)Label1);
		((Control)this).Controls.Add((Control)(object)txtOTP);
		((Control)this).Controls.Add((Control)(object)BtnGetOTP);
		((Control)this).Controls.Add((Control)(object)LBL_OTP);
		((Form)this).MaximizeBox = false;
		((Form)this).MinimizeBox = false;
		((Control)this).Name = "GET_and_VERIFY_OTP";
		((Form)this).Text = "OTP Verification";
		((Control)this).ResumeLayout(false);
		((Control)this).PerformLayout();
	}

	private void GET_and_VERIFY_OTP_Load(object sender, EventArgs e)
	{
		LBL_OTP.Text = "OPT Expired or Not Found";
		((Control)LBL_OTP).ForeColor = Color.Red;
		Module1.OTP_exit = false;
	}

	private void btnExit_Click(object sender, EventArgs e)
	{
		Module1.OTP_exit = true;
		((Form)this).Close();
		((Component)this).Dispose();
	}

	private void BtnVerifyOTP_Click(object sender, EventArgs e)
	{
		//IL_01d8: Unknown result type (might be due to invalid IL or missing references)
		//IL_01df: Expected Obj, but got Unknown
		if (Operators.CompareString(txtOTP.Text, "", TextCompare: false) == 0)
		{
			Interaction.MsgBox("Enter OTP", MsgBoxStyle.Information);
			((Control)txtOTP).Focus();
			return;
		}
		string text = "OTPVERIFY";
		string gSTIN = Module1.GSTIN1;
		string portalUserName = Module1.PortalUserName;
		string text2 = txtOTP.Text;
		string text3 = "1";
		string s = "{\"action\":\"" + text + "\",\"gstin\":\"" + gSTIN + "\",\"gst_username\":\"" + portalUserName + "\",\"otp\":\"" + text2 + "\"}";
		string uriString = "https://pro.mastersindia.co/taxpayerapis/authenticate";
		Uri uri = new Uri(uriString);
		byte[] bytes = Encoding.UTF8.GetBytes(s);
		string text4 = SendRequest_AUTH_TOCKEN(uri, bytes, "application/json", "POST");
		if (Operators.CompareString(text4, "", TextCompare: false) != 0)
		{
			string text5 = JsonConvert.SerializeObject(text4);
			JObject jObject = JObject.Parse(text4);
			string left = jObject["status"].ToString().Replace("\"", "");
			if (Operators.CompareString(left, "1", TextCompare: false) == 0)
			{
				Interaction.MsgBox(jObject["message"].ToString().Replace("\"", ""));
				DateTime dateTime = DateAndTime.Today.Date.AddDays(30.0);
				string text6 = "Update OtherSettings Set LabourRt='" + txtOTP.Text + "',tmpdttm=#" + dateTime.ToString("dd/MMM/yyyy") + "#,LabourRtVry='1'";
				OleDbCommand val = new OleDbCommand(text6, Module1.con);
				val.ExecuteNonQuery();
			}
			else
			{
				Interaction.MsgBox(jObject["message"].ToString().Replace("\"", ""));
			}
		}
	}

	private void BtnGetOTP_Click(object sender, EventArgs e)
	{
		string text = "OTPREQUEST";
		string gSTIN = Module1.GSTIN1;
		string portalUserName = Module1.PortalUserName;
		string s = "{\"action\":\"" + text + "\",\"gstin\":\"" + gSTIN + "\",\"gst_username\":\"" + portalUserName + "\"}";
		string uriString = "https://gstapi.in/taxpayerapis/authenticate";
		Uri uri = new Uri(uriString);
		byte[] bytes = Encoding.UTF8.GetBytes(s);
		string text2 = SendRequest_AUTH_TOCKEN(uri, bytes, "application/json", "POST");
		if (Operators.CompareString(text2, "", TextCompare: false) != 0)
		{
			string text3 = JsonConvert.SerializeObject(text2);
			JObject jObject = JObject.Parse(text2);
			string left = jObject["status"].ToString().Replace("\"", "");
			if (Operators.CompareString(left, "1", TextCompare: false) == 0)
			{
				Interaction.MsgBox(jObject["message"].ToString().Replace("\"", ""));
				LBL_OTP.Text = "OTP Sent, Please Enter OTP and Click on Verify OTP";
				((Control)LBL_OTP).ForeColor = Color.Green;
			}
			else
			{
				Interaction.MsgBox(jObject["message"].ToString().Replace("\"", ""));
			}
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
}
