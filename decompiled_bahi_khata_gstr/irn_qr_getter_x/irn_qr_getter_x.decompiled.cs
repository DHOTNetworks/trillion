using System;
using System.CodeDom.Compiler;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Configuration;
using System.Data;
using System.Data.Common;
using System.Data.OleDb;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Resources;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using AxAcroPDFLib;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.ApplicationServices;
using Microsoft.VisualBasic.CompilerServices;
using Microsoft.VisualBasic.Devices;
using Newtonsoft.Json.Linq;
using ZXing;
using irn_qr_getter.My;

[assembly: CompilationRelaxations(8)]
[assembly: RuntimeCompatibility(WrapNonExceptionThrows = true)]
[assembly: Debuggable(DebuggableAttribute.DebuggingModes.Default | DebuggableAttribute.DebuggingModes.DisableOptimizations | DebuggableAttribute.DebuggingModes.IgnoreSymbolStoreSequencePoints | DebuggableAttribute.DebuggingModes.EnableEditAndContinue)]
[assembly: AssemblyTitle("irn_qr_getter")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyCompany("")]
[assembly: AssemblyProduct("irn_qr_getter")]
[assembly: AssemblyCopyright("Copyright ©  2021")]
[assembly: AssemblyTrademark("")]
[assembly: ComVisible(false)]
[assembly: Guid("4349004e-0f85-4e14-9646-5940642e2007")]
[assembly: AssemblyFileVersion("1.0.0.0")]
[assembly: TargetFramework(".NETFramework,Version=v4.5", FrameworkDisplayName = ".NET Framework 4.5")]
[assembly: AssemblyVersion("1.0.0.0")]
namespace irn_qr_getter.My
{
	[GeneratedCode("MyTemplate", "11.0.0.0")]
	[EditorBrowsable(EditorBrowsableState.Never)]
	internal class MyApplication : ConsoleApplicationBase
	{
	}
	[GeneratedCode("MyTemplate", "11.0.0.0")]
	[EditorBrowsable(EditorBrowsableState.Never)]
	internal class MyComputer : Computer
	{
		[DebuggerHidden]
		[EditorBrowsable(EditorBrowsableState.Never)]
		public MyComputer()
		{
		}
	}
	[StandardModule]
	[HideModuleName]
	[GeneratedCode("MyTemplate", "11.0.0.0")]
	internal sealed class MyProject
	{
		[EditorBrowsable(EditorBrowsableState.Never)]
		[MyGroupCollection("System.Windows.Forms.Form", "Create__Instance__", "Dispose__Instance__", "My.MyProject.Forms")]
		internal sealed class MyForms
		{
			[ThreadStatic]
			private static Hashtable m_FormBeingCreated;

			[EditorBrowsable(EditorBrowsableState.Never)]
			public Form1 m_Form1;

			[EditorBrowsable(EditorBrowsableState.Never)]
			public Form2 m_Form2;

			public Form1 Form1
			{
				[DebuggerHidden]
				get
				{
					m_Form1 = Create__Instance__(m_Form1);
					return m_Form1;
				}
				[DebuggerHidden]
				set
				{
					if (value != m_Form1)
					{
						if (value != null)
						{
							throw new ArgumentException("Property can only be set to Nothing");
						}
						Dispose__Instance__(ref m_Form1);
					}
				}
			}

			public Form2 Form2
			{
				[DebuggerHidden]
				get
				{
					m_Form2 = Create__Instance__(m_Form2);
					return m_Form2;
				}
				[DebuggerHidden]
				set
				{
					if (value != m_Form2)
					{
						if (value != null)
						{
							throw new ArgumentException("Property can only be set to Nothing");
						}
						Dispose__Instance__(ref m_Form2);
					}
				}
			}

			[DebuggerHidden]
			private static T Create__Instance__<T>(T Instance) where T : Form, new()
			{
				if (Instance == null || ((Control)Instance).IsDisposed)
				{
					if (m_FormBeingCreated != null)
					{
						if (m_FormBeingCreated.ContainsKey(typeof(T)))
						{
							throw new InvalidOperationException(Utils.GetResourceString("WinForms_RecursiveFormCreate"));
						}
					}
					else
					{
						m_FormBeingCreated = new Hashtable();
					}
					m_FormBeingCreated.Add(typeof(T), null);
					try
					{
						return new T();
					}
					catch (TargetInvocationException ex) when (((Func<bool>)delegate
					{
						// Could not convert BlockContainer to single expression
						ProjectData.SetProjectError(ex);
						return ex.InnerException != null;
					}).Invoke())
					{
						string resourceString = Utils.GetResourceString("WinForms_SeeInnerException", ex.InnerException.Message);
						throw new InvalidOperationException(resourceString, ex.InnerException);
					}
					finally
					{
						m_FormBeingCreated.Remove(typeof(T));
					}
				}
				return Instance;
			}

			[DebuggerHidden]
			private void Dispose__Instance__<T>(ref T instance) where T : Form
			{
				((Component)instance/*cast due to constrained. prefix*/).Dispose();
				instance = default;
			}

			[DebuggerHidden]
			[EditorBrowsable(EditorBrowsableState.Never)]
			public MyForms()
			{
			}

			[EditorBrowsable(EditorBrowsableState.Never)]
			public override bool Equals(object o)
			{
				return base.Equals(RuntimeHelpers.GetObjectValue(o));
			}

			[EditorBrowsable(EditorBrowsableState.Never)]
			public override int GetHashCode()
			{
				return base.GetHashCode();
			}

			[EditorBrowsable(EditorBrowsableState.Never)]
			internal new Type GetType()
			{
				return typeof(MyForms);
			}

			[EditorBrowsable(EditorBrowsableState.Never)]
			public override string ToString()
			{
				return base.ToString();
			}
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		[MyGroupCollection("System.Web.Services.Protocols.SoapHttpClientProtocol", "Create__Instance__", "Dispose__Instance__", "")]
		internal sealed class MyWebServices
		{
			[EditorBrowsable(EditorBrowsableState.Never)]
			[DebuggerHidden]
			public override bool Equals(object o)
			{
				return base.Equals(RuntimeHelpers.GetObjectValue(o));
			}

			[EditorBrowsable(EditorBrowsableState.Never)]
			[DebuggerHidden]
			public override int GetHashCode()
			{
				return base.GetHashCode();
			}

			[EditorBrowsable(EditorBrowsableState.Never)]
			[DebuggerHidden]
			internal new Type GetType()
			{
				return typeof(MyWebServices);
			}

			[EditorBrowsable(EditorBrowsableState.Never)]
			[DebuggerHidden]
			public override string ToString()
			{
				return base.ToString();
			}

			[DebuggerHidden]
			private static T Create__Instance__<T>(T instance) where T : new()
			{
				if (instance == null)
				{
					return new T();
				}
				return instance;
			}

			[DebuggerHidden]
			private void Dispose__Instance__<T>(ref T instance)
			{
				instance = default;
			}

			[DebuggerHidden]
			[EditorBrowsable(EditorBrowsableState.Never)]
			public MyWebServices()
			{
			}
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		[ComVisible(false)]
		internal sealed class ThreadSafeObjectProvider<T> where T : new()
		{
			[CompilerGenerated]
			[ThreadStatic]
			private static T m_ThreadStaticValue;

			internal T GetInstance
			{
				[DebuggerHidden]
				get
				{
					if (m_ThreadStaticValue == null)
					{
						m_ThreadStaticValue = new T();
					}
					return m_ThreadStaticValue;
				}
			}

			[DebuggerHidden]
			[EditorBrowsable(EditorBrowsableState.Never)]
			public ThreadSafeObjectProvider()
			{
			}
		}

		private static readonly ThreadSafeObjectProvider<MyComputer> m_ComputerObjectProvider = new ThreadSafeObjectProvider<MyComputer>();

		private static readonly ThreadSafeObjectProvider<MyApplication> m_AppObjectProvider = new ThreadSafeObjectProvider<MyApplication>();

		private static readonly ThreadSafeObjectProvider<User> m_UserObjectProvider = new ThreadSafeObjectProvider<User>();

		private static ThreadSafeObjectProvider<MyForms> m_MyFormsObjectProvider = new ThreadSafeObjectProvider<MyForms>();

		private static readonly ThreadSafeObjectProvider<MyWebServices> m_MyWebServicesObjectProvider = new ThreadSafeObjectProvider<MyWebServices>();

		[HelpKeyword("My.Computer")]
		internal static MyComputer Computer
		{
			[DebuggerHidden]
			get
			{
				return m_ComputerObjectProvider.GetInstance;
			}
		}

		[HelpKeyword("My.Application")]
		internal static MyApplication Application
		{
			[DebuggerHidden]
			get
			{
				return m_AppObjectProvider.GetInstance;
			}
		}

		[HelpKeyword("My.User")]
		internal static User User
		{
			[DebuggerHidden]
			get
			{
				return m_UserObjectProvider.GetInstance;
			}
		}

		[HelpKeyword("My.Forms")]
		internal static MyForms Forms
		{
			[DebuggerHidden]
			get
			{
				return m_MyFormsObjectProvider.GetInstance;
			}
		}

		[HelpKeyword("My.WebServices")]
		internal static MyWebServices WebServices
		{
			[DebuggerHidden]
			get
			{
				return m_MyWebServicesObjectProvider.GetInstance;
			}
		}
	}
}
namespace irn_qr_getter.My.Resources
{
	[StandardModule]
	[GeneratedCode("System.Resources.Tools.StronglyTypedResourceBuilder", "16.0.0.0")]
	[DebuggerNonUserCode]
	[CompilerGenerated]
	[HideModuleName]
	internal sealed class Resources
	{
		private static ResourceManager resourceMan;

		private static CultureInfo resourceCulture;

		[EditorBrowsable(EditorBrowsableState.Advanced)]
		internal static ResourceManager ResourceManager
		{
			get
			{
				if (object.ReferenceEquals(resourceMan, null))
				{
					ResourceManager resourceManager = new ResourceManager("irn_qr_getter.Resources", typeof(Resources).Assembly);
					resourceMan = resourceManager;
				}
				return resourceMan;
			}
		}

		[EditorBrowsable(EditorBrowsableState.Advanced)]
		internal static CultureInfo Culture
		{
			get
			{
				return resourceCulture;
			}
			set
			{
				resourceCulture = value;
			}
		}
	}
}
namespace irn_qr_getter.My
{
	[CompilerGenerated]
	[GeneratedCode("Microsoft.VisualStudio.Editors.SettingsDesigner.SettingsSingleFileGenerator", "16.8.1.0")]
	[EditorBrowsable(EditorBrowsableState.Advanced)]
	internal sealed class MySettings : ApplicationSettingsBase
	{
		private static MySettings defaultInstance = (MySettings)(object)SettingsBase.Synchronized((SettingsBase)(object)new MySettings());

		public static MySettings Default => defaultInstance;
	}
	[StandardModule]
	[HideModuleName]
	[DebuggerNonUserCode]
	[CompilerGenerated]
	internal sealed class MySettingsProperty
	{
		[HelpKeyword("My.Settings")]
		internal static MySettings Settings => MySettings.Default;
	}
}
namespace irn_qr_getter
{
	[DesignerGenerated]
	public class Form1 : Form
	{
		public OleDbConnection con;

		private OleDbDataAdapter da;

		private DataSet ds;

		private DataTable dt;

		private OleDbDataAdapter da1;

		private DataSet ds1;

		private DataTable dt1;

		private OleDbCommand cmd;

		private bool open_browser;

		private string Nw_IRN;

		private string Nw_GST;

		private string Nw_CDKey;

		private string Nw_EInvUserName;

		private string Nw_EInvPassword;

		private string Nw_EFUserName;

		private string Nw_EFPassword;

		private string NW_EWBUserName;

		private string NW_EWBPassword;

		private string EWay_Nw_IRN;

		private string CnlRsn;

		private string CnlRem;

		private string database_name;

		private string update_tblnm;

		private string IRN_GET_URL;

		private string GET_INV_URL;

		private string CANCEL_URL;

		private string QRJpgImageName;

		private string VoucherDate;

		private string voucherNumber;

		private string transtype;

		private string party_api_bal;

		private string Check_bal;

		private string EWAY_JSON_STR;

		private string yr;

		private string mnth;

		private string EWayBillNum;

		private string URL1;

		private string title;

		private string action;

		private string sinv;

		private bool field_exists;

		private string PDF_File;

		private string Entered_distance;

		private string new_param;

		private string json_String;

		private IContainer components;

		[field: AccessedThroughProperty("PictureBox2")]
		internal virtual PictureBox PictureBox2
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("TextBox2")]
		internal virtual TextBox TextBox2
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("TextBox1")]
		internal virtual TextBox TextBox1
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("PictureBox1")]
		internal virtual PictureBox PictureBox1
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("BtnAPIBal")]
		internal virtual Button BtnAPIBal
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = BtnAPIBal_Click;
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

		[field: AccessedThroughProperty("BtnCheckBal")]
		internal virtual Button BtnCheckBal
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = BtnCheckBal_Click;
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

		[field: AccessedThroughProperty("btnEWay")]
		internal virtual Button btnEWay
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = btnEWay_Click;
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

		[field: AccessedThroughProperty("btnEInvoice")]
		internal virtual Button btnEInvoice
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = btnEInvoice_Click;
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

		[field: AccessedThroughProperty("btnDistance")]
		internal virtual Button btnDistance
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = btnDistance_Click;
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

		[field: AccessedThroughProperty("searchGSTIN")]
		internal virtual Button searchGSTIN
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = NewApis_Click;
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

		[field: AccessedThroughProperty("BtnSave")]
		internal virtual Button BtnSave
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = BtnSave_Click;
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
			//IL_001b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0025: Expected Obj, but got Unknown
			//IL_0026: Unknown result type (might be due to invalid IL or missing references)
			//IL_0030: Expected Obj, but got Unknown
			((Form)this).Load += Form1_Load;
			con = new OleDbConnection();
			cmd = new OleDbCommand();
			InitializeComponent();
		}

		private void Form1_Load(object sender, EventArgs e)
		{
			//IL_019d: Unknown result type (might be due to invalid IL or missing references)
			//IL_01a7: Expected Obj, but got Unknown
			//IL_01fd: Unknown result type (might be due to invalid IL or missing references)
			//IL_0207: Expected Obj, but got Unknown
			title = "Automatic E-Invoice/E-Way Utility";
			if (Module1.Local)
			{
				database_name = "D:\\Bahi-Khata10 Demo\\Data\\Data.002";
				Module1.Main_TABLE = "TempEWayTableByIRN";
			}
			else
			{
				database_name = Module1.db_path;
			}
			string path = Application.StartupPath + "\\Myparams.txt";
			StreamWriter streamWriter = new StreamWriter(path);
			string text = "DB path : " + Module1.db_path + "\r\n";
			text = text + "Main Table : " + Module1.Main_TABLE;
			streamWriter.Write(text);
			streamWriter.Close();
			conn(database_name);
			action = "";
			update_tblnm = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyQRCodeTable");
			action = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyRequireAPI");
			VoucherDate = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyVoucherDate");
			voucherNumber = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyvoucherNumber");
			transtype = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "Mytranstype");
			if (!string.IsNullOrEmpty(update_tblnm))
			{
				field_exists = false;
				field_exists_or_not("EInvAckNo", update_tblnm);
				if (!field_exists)
				{
					string text2 = "ALTER TABLE " + update_tblnm + " ADD COLUMN EInvAckNo Text(255)";
					cmd = new OleDbCommand(text2, con);
					cmd.ExecuteNonQuery();
				}
				field_exists = false;
				field_exists_or_not("EInvAckDate", update_tblnm);
				if (!field_exists)
				{
					string text3 = "ALTER TABLE " + update_tblnm + " ADD COLUMN EInvAckDate Text(255)";
					cmd = new OleDbCommand(text3, con);
					cmd.ExecuteNonQuery();
				}
			}
			if ((Operators.CompareString(action, "", TextCompare: false) == 0) | (Operators.CompareString(action, null, TextCompare: false) == 0))
			{
				Interaction.MsgBox("No Action Defined For Eway or EInvoice Operation", MsgBoxStyle.Critical, "Bahi-Khata");
				Application.Exit();
			}
			else if (action.Contains("einvoice"))
			{
				BtnAPIBal_Click(RuntimeHelpers.GetObjectValue(sender), e);
				btnEInvoice_Click(RuntimeHelpers.GetObjectValue(sender), e);
				Application.Exit();
			}
			else if (action.Contains("eway"))
			{
				BtnAPIBal_Click(RuntimeHelpers.GetObjectValue(sender), e);
				btnEWay_Click(RuntimeHelpers.GetObjectValue(sender), e);
				Application.Exit();
			}
			else if (action.Contains("GSTNstatus"))
			{
				NewApis_Click(RuntimeHelpers.GetObjectValue(sender), e);
				Application.Exit();
			}
			else if (Operators.CompareString(action, "editdistance", TextCompare: false) == 0)
			{
				Distance_UPDATE();
				Application.Exit();
			}
			else if (action.Contains("distance"))
			{
				BtnAPIBal_Click(RuntimeHelpers.GetObjectValue(sender), e);
				btnDistance_Click(RuntimeHelpers.GetObjectValue(sender), e);
				Application.Exit();
			}
		}

		private void DEDUCT_API()
		{
			string text = "68c37f4121e93dc4b49b31820dd5602d7";
			string text2 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyYourID");
			string text3 = ((Operators.CompareString(new_param, "mydistance", TextCompare: false) != 0) ? get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyRequireAPI") : new_param);
			string text4 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "Gstin");
			if (Operators.CompareString(text3, "", TextCompare: false) == 0)
			{
				Interaction.MsgBox("No Data Found In Table\r\n Table : " + Module1.Main_TABLE, MsgBoxStyle.Critical, title);
				return;
			}
			if (text3.Contains("einvoice"))
			{
				text3 = "einvoice";
			}
			else if (text3.Contains("eway"))
			{
				text3 = "eway";
			}
			else if (text3.Contains("distance"))
			{
				text3 = "distance";
			}
			string text5 = "1";
			string args = "{\"apikey\":\"" + text + "\",\"party_id\":\"" + text2 + "\",\"api_parameter\":\"" + text3 + "\",\"debit_value\":\"" + text5 + "\",\"gst\":\"" + text4 + "\"}";
			string text6 = "http://kamrasoftwares.com/epanel/web-service/index.php?service=debitTransactionBalance";
			text6 = "http://kamrasoftwares.com/epanel/web-service/index.php?service=debitTransactionBalance";
			upload_data_9(text6, args);
		}

		private void upload_data_9(string url, string args)
		{
			int try0001_dispatch = -1;
			int num2 = default;
			JObject jObject = default;
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
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							jObject = JObject.Parse(text);
						}
						if (Operators.CompareString(jObject["success"].ToString(), "1", TextCompare: false) != 0 && Operators.CompareString(jObject["success"].ToString(), "0", TextCompare: false) == 0)
						{
							string text2 = jObject["msg"].ToString();
							text2 = text2.Replace("\"", "");
							Interaction.MsgBox("Ehhh !!!\r\n\r\n\r\n" + text2 + "\r\n\r\n" + args, MsgBoxStyle.Critical, "Check API");
						}
						break;
					}
					case 315:
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
						Interaction.MsgBox("Error Occured IRN upload_data_9 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 315;
					continue;
				}
				throw ProjectData.CreateProjectError(-2146828237);
			}
			if (num != 0)
			{
				ProjectData.ClearProjectError();
			}
		}

		public string get_single_value(string get_field_nm, string tbl_nm, string check_field, string val1)
		{
			//IL_0014: Unknown result type (might be due to invalid IL or missing references)
			//IL_001a: Expected Obj, but got Unknown
			//IL_006d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0073: Expected Obj, but got Unknown
			conn(database_name);
			Application.DoEvents();
			OleDbDataAdapter val2 = new OleDbDataAdapter();
			DataSet dataSet = new DataSet();
			string text = "select " + get_field_nm + " from " + tbl_nm + " where " + check_field + "='" + val1 + "'";
			val2 = new OleDbDataAdapter(text, con);
			dataSet = new DataSet();
			((DbDataAdapter)(object)val2).Fill(dataSet);
			string result = default;
			if (dataSet.Tables[0].Rows.Count > 0)
			{
				result = dataSet.Tables[0].Rows[0].ItemArray[0].ToString();
			}
			CloseConn();
			return result;
		}

		public void conn(string dbnm)
		{
			if (con.State != ConnectionState.Open)
			{
				con.ConnectionString = "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + dbnm + " ;Jet OLEDB:Database Password=jay_hanuman;";
				con.Open();
			}
		}

		public void CloseConn()
		{
		}

		private void btnEInvoice_Click(object sender, EventArgs e)
		{
			//IL_031f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0326: Expected Obj, but got Unknown
			//IL_02fc: Unknown result type (might be due to invalid IL or missing references)
			//IL_0306: Expected Obj, but got Unknown
			//IL_0473: Unknown result type (might be due to invalid IL or missing references)
			//IL_047d: Expected Obj, but got Unknown
			//IL_04cc: Unknown result type (might be due to invalid IL or missing references)
			//IL_04d6: Expected Obj, but got Unknown
			conn(database_name);
			action = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyRequireAPI");
			if (Operators.CompareString(party_api_bal, "", TextCompare: false) == 0)
			{
				JSON_Generate();
				Write_Json_file(TextBox1.Text);
				return;
			}
			if (Operators.CompareString(action, "einvoice", TextCompare: false) == 0)
			{
				JSON_Generate();
				string text2 = default;
				string text3 = default;
				string text4 = default;
				string text = "{\"mobileno\":\"" + text2 + "\",\"email\":\"" + text3 + "\",\"pc\":\"" + text4 + "\"}";
				text = TextBox1.Text;
				URL1 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyURL");
				string uRL = URL1;
				Write_Json_file(text);
				upload_data_1(uRL, text);
				string text5 = TextBox2.Text;
				text5 = text5.Replace("[", "");
				text5 = text5.Replace("]", "");
				Write_Txt_file(text5);
				JObject jObject = JObject.Parse(text5);
				string text6 = jObject["Irn"].ToString();
				string text7 = jObject["SignedQRCode"].ToString();
				string text8 = jObject["SignedInvoice"].ToString();
				string text9 = jObject["AckNo"].ToString();
				string text10 = jObject["AckDate"].ToString();
				text9 = text9.Replace("\"", "");
				text10 = text10.Replace("\"", "");
				string text11 = jObject["ErrorMessage"].ToString();
				text11 = text11.Replace("\"", "");
				if (Operators.CompareString(text11, "", TextCompare: false) != 0)
				{
					Interaction.MsgBox("Error Occurred\r\n\r\n" + text11 + "\r\n\r\n" + text5, MsgBoxStyle.Critical, title);
					return;
				}
				DEDUCT_API();
				text6 = text6.Replace("\"", "");
				text7 = text7.Replace("\"", "");
				text8 = text8.Replace("\"", "");
				sinv = text8;
				Nw_IRN = text6;
				Write_IRN(Nw_IRN);
				QRJpgImageName = Application.StartupPath + "\\IRN.jpg";
				GenerateEinvoiceQRCode(text7, "QR");
				field_exists = false;
				field_exists_or_not("EInvStatus", update_tblnm);
				if (!field_exists)
				{
					string text12 = "ALTER TABLE " + update_tblnm + " ADD COLUMN EInvStatus Text";
					cmd = new OleDbCommand(text12, con);
					cmd.ExecuteNonQuery();
				}
				Bitmap val = (Bitmap)PictureBox1.Image;
				MemoryStream memoryStream = new MemoryStream();
				((Image)val).Save((Stream)memoryStream, ImageFormat.Bmp);
				byte[] buffer = memoryStream.GetBuffer();
				string text13 = ((Operators.CompareString(transtype, "Jrnl", TextCompare: false) != 0) ? ("Update " + update_tblnm + " set QRCode=@img,IRNNo='" + Nw_IRN + "',EInvStatus='Live',EInvAckNo='" + text9 + "',EInvAckDate='" + text10 + "' WHERE VoucherDate=#" + VoucherDate + "# and VoucherNumber=" + voucherNumber + " and TransType='" + transtype + "'") : (("Update " + update_tblnm + " set QRCode=@img,IRNNo='" + Nw_IRN + "',EInvStatus='Live',EInvAckNo='" + text9 + "',EInvAckDate='" + text10 + "' WHERE VoucherDate=#" + VoucherDate + "# and VoucherNumber=" + voucherNumber) ?? ""));
				cmd = new OleDbCommand(text13, con);
				cmd.Parameters.Add("@img", (object)SqlDbType.Image).Value = buffer;
				cmd.ExecuteNonQuery();
				text13 = "Update TempPrintEInvoiceTable set fieldvalue='" + Nw_IRN + "' where fieldnm='Irn'";
				cmd = new OleDbCommand(text13, con);
				cmd.ExecuteNonQuery();
				open_browser = false;
				GET_INV_PDF_USING_IRN();
				open_browser = false;
				string value = Conversions.ToString((int)Interaction.MsgBox("Your E-Invoice is Successfully Generated\r\n\r\nIRN No. = " + Nw_IRN + "\r\n\r\nDo You Want To Print E-Invoice ?", MsgBoxStyle.YesNo | MsgBoxStyle.Question, title));
				if (Conversions.ToDouble(value) != 6.0)
				{
					Application.Exit();
					return;
				}
				string pDF_File = PDF_File;
				Process.Start(pDF_File);
			}
			if (Operators.CompareString(action, "reprinteinvoice", TextCompare: false) == 0)
			{
				open_browser = true;
				GET_INV_PDF_USING_IRN();
				Application.Exit();
			}
			else if (Operators.CompareString(action, "canceleinvoice", TextCompare: false) == 0)
			{
				CANCEL_EINVOICE();
				Application.Exit();
			}
		}

		private void Convert_PDF_fromBase64(string get_field_nm, string error_msg = "", string result = "")
		{
			//IL_010a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0110: Expected Obj, but got Unknown
			conn(database_name);
			string text2 = default;
			if (Operators.CompareString(update_tblnm, "", TextCompare: false) != 0)
			{
				string text = ((Operators.CompareString(transtype, "Jrnl", TextCompare: false) != 0) ? ("select " + get_field_nm + " From " + update_tblnm + " WHERE VoucherDate=#" + VoucherDate + "# and VoucherNumber=" + voucherNumber + " and TransType='" + transtype + "'") : (("select " + get_field_nm + " From " + update_tblnm + " WHERE VoucherDate=#" + VoucherDate + "# and VoucherNumber=" + voucherNumber) ?? ""));
				OleDbDataAdapter val = new OleDbDataAdapter(text, con);
				DataSet dataSet = new DataSet();
				((DbDataAdapter)(object)val).Fill(dataSet);
				if (dataSet.Tables[0].Rows.Count > 0)
				{
					text2 = dataSet.Tables[0].Rows[0].ItemArray[0].ToString();
				}
			}
			if (Operators.CompareString(text2, "", TextCompare: false) != 0)
			{
				if (Operators.CompareString(error_msg, "", TextCompare: false) != 0)
				{
					Interaction.MsgBox("PDF File Successfully Created", MsgBoxStyle.Information, "Print E-Invoice PDF");
				}
				string text3 = Application.StartupPath + "\\~temp~.pdf";
				Base64FileToPDF(text2, text3);
				if (Operators.CompareString(text3, "", TextCompare: false) != 0)
				{
					OPEN_PDF_in_browsers(text3);
				}
			}
			else
			{
				Interaction.MsgBox("Error Occurred\r\n\r\n" + error_msg + "\r\n\r\n" + result, MsgBoxStyle.Critical, title);
			}
		}

		private void CANCEL_EINVOICE()
		{
			//IL_001c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0026: Expected Obj, but got Unknown
			conn(database_name);
			string text = "SELECT * from TempCancelEInvoiceTable order by itemsrno";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			checked
			{
				int num = dt.Rows.Count - 1;
				string text2 = default;
				for (int i = 0; i <= num; i++)
				{
					if (Operators.CompareString(Strings.Left(dt.Rows[i]["FieldNm"].ToString(), 2), "My", TextCompare: false) != 0)
					{
						text2 = ((i != 0) ? (text2 + ",\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\"") : (text2 + "\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\""));
					}
				}
				text2 = "{" + text2 + "}";
				text2 = "\"Data\":[" + text2 + "]";
				text2 = "\"Push_Data_List\":{" + text2 + "}";
				text2 = "{" + text2 + "}";
				string text3 = text2;
				URL1 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyURL");
				string uRL = URL1;
				Write_Json_file(text3);
				upload_data_4(uRL, text3);
			}
		}

		private void GET_INV_PDF_USING_IRN()
		{
			//IL_001c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0026: Expected Obj, but got Unknown
			conn(database_name);
			string text = "SELECT * from TempPrintEInvoiceTable order by itemsrno";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			checked
			{
				int num = dt.Rows.Count - 1;
				string text2 = default;
				for (int i = 0; i <= num; i++)
				{
					if (Operators.CompareString(Strings.Left(dt.Rows[i]["FieldNm"].ToString(), 2), "My", TextCompare: false) != 0)
					{
						text2 = ((i != 0) ? (text2 + ",\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\"") : (text2 + "\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\""));
					}
				}
				if (Operators.CompareString(text2, "", TextCompare: false) != 0)
				{
					text2 = "{" + text2 + "}";
					string text3 = text2;
					string text4 = get_single_value("FieldValue", "TempPrintEInvoiceTable", "Fieldnm", "MyURL");
					string url = text4;
					Write_Json_file(text3);
					upload_data_2(url, text3);
				}
			}
		}

		private void field_exists_or_not(string field_nm, string tbl_nm)
		{
			//IL_002b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0035: Expected Obj, but got Unknown
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
							conn(database_name);
							ProjectData.ClearProjectError();
							num2 = 2;
							string text = "Select top 1 * from " + tbl_nm;
							da = new OleDbDataAdapter(text, con);
							ds = new DataSet();
							((DbDataAdapter)(object)da).Fill(ds);
							dt = ds.Tables[0];
							int num3 = dt.Columns.Count - 1;
							for (int i = 0; i <= num3; i++)
							{
								if (Operators.CompareString(Strings.UCase(dt.Columns[i].ColumnName), Strings.UCase(field_nm), TextCompare: false) == 0)
								{
									field_exists = true;
									break;
								}
							}
							break;
						}
						case 280:
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
							Interaction.MsgBox(Information.Err().Description + "\r\n\r\n" + con.ConnectionString);
						}
						CloseConn();
						break;
						end_IL_0001:;
					}
					catch (Exception ex) when ((num2 != 0) & (num == 0))
					{
						ProjectData.SetProjectError(ex);
						try0001_dispatch = 280;
						continue;
					}
					throw ProjectData.CreateProjectError(-2146828237);
				}
				if (num != 0)
				{
					ProjectData.ClearProjectError();
				}
			}
		}

		private void upload_data_12(string url, string args)
		{
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
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							TextBox2.Text = text;
						}
						break;
					}
					case 173:
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
						Interaction.MsgBox("Error Occured IRN upload_data_12 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 173;
					continue;
				}
				throw ProjectData.CreateProjectError(-2146828237);
			}
			if (num != 0)
			{
				ProjectData.ClearProjectError();
			}
		}

		private void upload_data_1(string url, string args)
		{
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
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							TextBox2.Text = text;
						}
						break;
					}
					case 173:
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
						Interaction.MsgBox("Error Occured IRN upload_data_1 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 173;
					continue;
				}
				throw ProjectData.CreateProjectError(-2146828237);
			}
			if (num != 0)
			{
				ProjectData.ClearProjectError();
			}
		}

		private void upload_data_2(string url, string args)
		{
			//IL_006f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0079: Expected Obj, but got Unknown
			//IL_00cf: Unknown result type (might be due to invalid IL or missing references)
			//IL_00d9: Expected Obj, but got Unknown
			//IL_012f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0139: Expected Obj, but got Unknown
			//IL_03c3: Unknown result type (might be due to invalid IL or missing references)
			//IL_03cd: Expected Obj, but got Unknown
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
						if (Operators.CompareString(update_tblnm, "", TextCompare: false) != 0)
						{
							field_exists = false;
							field_exists_or_not("EInvPDF", update_tblnm);
							if (!field_exists)
							{
								string text = "ALTER TABLE " + update_tblnm + " ADD COLUMN EInvPDF memo";
								cmd = new OleDbCommand(text, con);
								cmd.ExecuteNonQuery();
							}
							field_exists = false;
							field_exists_or_not("EInvAckNo", update_tblnm);
							if (!field_exists)
							{
								string text2 = "ALTER TABLE " + update_tblnm + " ADD COLUMN EInvAckNo text";
								cmd = new OleDbCommand(text2, con);
								cmd.ExecuteNonQuery();
							}
							field_exists = false;
							field_exists_or_not("EInvAckDate", update_tblnm);
							if (!field_exists)
							{
								string text3 = "ALTER TABLE " + update_tblnm + " ADD COLUMN EInvAckDate text";
								cmd = new OleDbCommand(text3, con);
								cmd.ExecuteNonQuery();
							}
						}
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text4 = SendRequest(uri, bytes, "application/json", "POST");
						text4 = text4.Replace("[", "");
						text4 = text4.Replace("]", "");
						PDF_File = "";
						JObject jObject = JObject.Parse(text4);
						Write_Txt_file(text4);
						if (Operators.CompareString(text4, "", TextCompare: false) != 0)
						{
							string text5 = jObject["ErrorMessage"].ToString();
							text5 = text5.Replace("\"", "");
							if (Operators.CompareString(text5, "", TextCompare: false) != 0)
							{
								if (text5.Contains("IRN details cannot be provided as it is generated more than"))
								{
									Convert_PDF_fromBase64("EInvPDF", text5, text4);
								}
								else
								{
									Interaction.MsgBox("Error Occurred\r\n\r\n" + text5 + "\r\n\r\n" + text4, MsgBoxStyle.Critical, title);
								}
								goto end_IL_0001;
							}
							PDF_File = jObject["File"].ToString();
							PDF_File = PDF_File.Replace("\"", "");
						}
						string fileName = Application.StartupPath + "\\~temp~.pdf";
						WebClient webClient = new WebClient();
						webClient.DownloadFile(PDF_File, fileName);
						string text6 = ConvertFileToBase64(fileName);
						if (Operators.CompareString(update_tblnm, "", TextCompare: false) != 0)
						{
							string text7 = ((Operators.CompareString(transtype, "Jrnl", TextCompare: false) != 0) ? ("Update " + update_tblnm + " set EInvPDF='" + text6 + "' WHERE VoucherDate=#" + VoucherDate + "# and VoucherNumber=" + voucherNumber + " and TransType='" + transtype + "'") : (("Update " + update_tblnm + " set EInvPDF='" + text6 + "' WHERE VoucherDate=#" + VoucherDate + "# and VoucherNumber=" + voucherNumber) ?? ""));
							cmd = new OleDbCommand(text7, con);
							cmd.Parameters.Add("@img", (object)SqlDbType.Image).Value = bytes;
							cmd.ExecuteNonQuery();
						}
						DEDUCT_API();
						if (open_browser)
						{
							string pDF_File = PDF_File;
							Process.Start(pDF_File);
						}
						break;
					}
					case 1141:
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
						Interaction.MsgBox("Error Occured IRN upload_data_2 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001_2:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 1141;
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

		private void OPEN_PDF_in_browsers(string fname)
		{
			try
			{
				fname = "file:///" + fname.Replace(" ", "%20");
				fname = fname.Replace("\\", "/");
				Process.Start(string.Format("{0}.exe", "chrome"), fname);
			}
			catch (Exception ex)
			{
				ProjectData.SetProjectError(ex);
				Exception ex2 = ex;
				if (ex2.ToString().Contains("The system cannot find the file specified"))
				{
					try
					{
						Process.Start("Firefox.exe", fname);
					}
					catch (Exception ex3)
					{
						ProjectData.SetProjectError(ex3);
						Exception ex4 = ex3;
						if (ex4.ToString().Contains("The system cannot find the file specified"))
						{
							try
							{
								Process.Start(fname);
							}
							catch (Exception ex5)
							{
								ProjectData.SetProjectError(ex5);
								Exception ex6 = ex5;
								Interaction.MsgBox("Error Occured : \r\n" + ex2.ToString());
								ProjectData.ClearProjectError();
							}
						}
						ProjectData.ClearProjectError();
					}
				}
				ProjectData.ClearProjectError();
			}
		}

		public string ConvertFileToBase64(string fileName)
		{
			string result = "";
			if (((ServerComputer)MyProject.Computer).FileSystem.FileExists(fileName))
			{
				using FileStream fileStream = new FileStream(fileName, FileMode.Open);
				BinaryReader binaryReader = new BinaryReader(fileStream);
				byte[] inArray = binaryReader.ReadBytes(checked((int)fileStream.Length));
				result = Convert.ToBase64String(inArray);
				fileStream.Close();
			}
			return result;
		}

		private void Base64FileToPDF(string base64, string save_file_location)
		{
			byte[] array = Convert.FromBase64String(base64);
			FileStream fileStream = File.Create(save_file_location);
			fileStream.Write(array, 0, array.Length);
			fileStream.Flush();
			fileStream.Close();
		}

		private void upload_data_31(string url, string args)
		{
			//IL_008a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0094: Expected Obj, but got Unknown
			int try0001_dispatch = -1;
			int num2 = default;
			JObject jObject = default;
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
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							jObject = JObject.Parse(text);
						}
						if (Operators.CompareString(jObject["success"].ToString(), "1", TextCompare: false) == 0)
						{
							string text2 = "Insert into TempDistance(ItemSrNo,FieldNm,FieldValue) values(1,'Distance','editdistancesuccess')";
							cmd = new OleDbCommand(text2, con);
							cmd.ExecuteNonQuery();
						}
						else if (Operators.CompareString(jObject["success"].ToString(), "0", TextCompare: false) == 0)
						{
							string text3 = jObject["msg"].ToString();
							text3 = text3.Replace("\"", "");
							Interaction.MsgBox("Ehhh !!!\r\n\r\n\r\n" + text3 + "\r\n\r\n" + args, MsgBoxStyle.Critical, "Check API");
						}
						break;
					}
					case 353:
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
						Interaction.MsgBox("Error Occured IRN upload_data_31 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 353;
					continue;
				}
				throw ProjectData.CreateProjectError(-2146828237);
			}
			if (num != 0)
			{
				ProjectData.ClearProjectError();
			}
		}

		private void upload_data_3(string url, string args)
		{
			int try0001_dispatch = -1;
			int num2 = default;
			JObject jObject = default;
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
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							jObject = JObject.Parse(text);
						}
						if (Operators.CompareString(jObject["success"].ToString(), "1", TextCompare: false) == 0)
						{
							party_api_bal = jObject["data"]["balance"].ToString();
							party_api_bal = party_api_bal.Replace("\"", "");
						}
						else if (Operators.CompareString(jObject["success"].ToString(), "0", TextCompare: false) == 0)
						{
							party_api_bal = "";
							string text2 = jObject["msg"].ToString();
							text2 = text2.Replace("\"", "");
							Interaction.MsgBox("Ehhh !!!\r\n\r\n\r\n" + text2 + "\r\n\r\n" + args, MsgBoxStyle.Critical, "Check API");
						}
						break;
					}
					case 386:
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
						Interaction.MsgBox("Error Occured IRN upload_data_3 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 386;
					continue;
				}
				throw ProjectData.CreateProjectError(-2146828237);
			}
			if (num != 0)
			{
				ProjectData.ClearProjectError();
			}
		}

		private void upload_data_4(string url, string args)
		{
			//IL_0158: Unknown result type (might be due to invalid IL or missing references)
			//IL_0162: Expected Obj, but got Unknown
			//IL_023b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0245: Expected Obj, but got Unknown
			//IL_028b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0295: Expected Obj, but got Unknown
			int try0001_dispatch = -1;
			int num2 = default;
			string text3 = default;
			string text4 = default;
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
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						text = text.Replace("[", "");
						text = text.Replace("]", "");
						JObject jObject = JObject.Parse(text);
						Write_Txt_file(text);
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							string text2 = jObject["ErrorMessage"].ToString();
							text2 = text2.Replace("\"", "");
							if (Operators.CompareString(text2, "", TextCompare: false) != 0)
							{
								Interaction.MsgBox("Error Occurred\r\n\r\n" + text2 + "\r\n\r\n" + text, MsgBoxStyle.Critical, title);
								goto end_IL_0001;
							}
							text3 = jObject["CancelDate"].ToString();
							text4 = jObject["Irn"].ToString();
						}
						field_exists = false;
						field_exists_or_not("EInvStatus", update_tblnm);
						if (!field_exists)
						{
							string text5 = "ALTER TABLE " + update_tblnm + " ADD COLUMN EInvStatus Text";
							cmd = new OleDbCommand(text5, con);
							cmd.ExecuteNonQuery();
						}
						string text6 = ((Operators.CompareString(transtype, "Jrnl", TextCompare: false) != 0) ? ("Update " + update_tblnm + " set EInvStatus='Cancel' WHERE VoucherDate=#" + VoucherDate + "# and VoucherNumber=" + voucherNumber + " and TransType='" + transtype + "'") : (("Update " + update_tblnm + " set EInvStatus='Cancel' WHERE VoucherDate=#" + VoucherDate + "# and VoucherNumber=" + voucherNumber) ?? ""));
						cmd = new OleDbCommand(text6, con);
						cmd.ExecuteNonQuery();
						if (Operators.CompareString(Nw_IRN, "", TextCompare: false) != 0)
						{
							text6 = "Update TempPrintEInvoiceTable set fieldvalue='" + Nw_IRN + "' where fieldnm='Irn'";
							cmd = new OleDbCommand(text6, con);
							cmd.ExecuteNonQuery();
						}
						open_browser = false;
						GET_INV_PDF_USING_IRN();
						DEDUCT_API();
						Interaction.MsgBox("Your E-Invoice With IRN No &" + text4 + " has been Successfully Cancelled\r\n\r\nCancel Date : " + text3, MsgBoxStyle.Information, title);
						break;
					}
					case 812:
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
						Interaction.MsgBox("Error Occured IRN upload_data_4 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001_2:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 812;
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

		private void GenerateEwayWithoutIrn()
		{
			//IL_0094: Unknown result type (might be due to invalid IL or missing references)
			//IL_009e: Expected Obj, but got Unknown
			//IL_0265: Unknown result type (might be due to invalid IL or missing references)
			//IL_026f: Expected Obj, but got Unknown
			//IL_0573: Unknown result type (might be due to invalid IL or missing references)
			//IL_057d: Expected Obj, but got Unknown
			//IL_05a9: Unknown result type (might be due to invalid IL or missing references)
			//IL_05b3: Expected Obj, but got Unknown
			//IL_04e5: Unknown result type (might be due to invalid IL or missing references)
			//IL_04ef: Expected Obj, but got Unknown
			conn(database_name);
			action = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyRequireAPI");
			if (Operators.CompareString(party_api_bal, "", TextCompare: false) == 0)
			{
				JSON_Generate();
				Write_Json_file(TextBox1.Text);
			}
			else if (Operators.CompareString(action, "ewaybyirn", TextCompare: false) == 0)
			{
				Application.DoEvents();
				string text = "SELECT * from TempEWayTableByIRN where ItemSrNo=0";
				da = new OleDbDataAdapter(text, con);
				ds = new DataSet();
				((DbDataAdapter)(object)da).Fill(ds);
				dt = ds.Tables[0];
				string text2 = "";
				checked
				{
					if (dt.Rows.Count > 0)
					{
						int num = dt.Rows.Count - 1;
						for (int i = 0; i <= num; i++)
						{
							text2 = ((!((Operators.CompareString(text2, "", TextCompare: false) == 0) | (Operators.CompareString(text2, null, TextCompare: false) == 0))) ? (text2 + ",\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\"") : (text2 + "\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\""));
						}
						EWAYWithoutIRNJsonGenerate(text2);
					}
					string eWAY_JSON_STR = EWAY_JSON_STR;
					Application.DoEvents();
					Thread.Sleep(10000);
					text = "SELECT FieldValue from " + Module1.Main_TABLE + " WHERE Fieldnm='MyURL'";
					da = new OleDbDataAdapter(text, con);
					ds = new DataSet();
					((DbDataAdapter)(object)da).Fill(ds);
					dt = ds.Tables[0];
					if (dt.Rows.Count > 0)
					{
						URL1 = dt.Rows[0].ItemArray[0].ToString();
					}
					string uRL = URL1;
					Write_Json_file(eWAY_JSON_STR);
					upload_data_6(uRL, eWAY_JSON_STR);
					string text3 = TextBox2.Text;
					if (Operators.CompareString(text3, "", TextCompare: false) == 0)
					{
						Interaction.MsgBox("Response Blank Received", MsgBoxStyle.Critical, "Bahi-Khata");
						Application.Exit();
						return;
					}
					text3 = text3.Replace("[", "");
					text3 = text3.Replace("]", "");
					text3 = text3.Replace("\\", "");
					text3 = text3.TrimEnd(new char[1] { '"' });
					text3 = text3.TrimStart(new char[1] { '"' });
					Write_Txt_file(text3);
					JObject jObject = JObject.Parse(text3);
					string text4 = jObject["EWayBill"].ToString();
					string text5 = jObject["Date"].ToString();
					string text6 = jObject["ErrorMessage"].ToString();
					text6 = text6.Replace("\"", "");
					if (Operators.CompareString(text6, "", TextCompare: false) != 0)
					{
						Interaction.MsgBox("Error Occurred\r\n\r\n" + text6 + "\r\n\r\n" + text3, MsgBoxStyle.Critical, title);
						return;
					}
					DEDUCT_API();
					text4 = text4.Replace("\"", "");
					text5 = text5.Replace("\"", "");
					EWayBillNum = text4;
					yr = Conversions.ToString(DateAndTime.Year(Conversions.ToDate(text5)));
					mnth = Conversions.ToString(DateAndTime.Month(Conversions.ToDate(text5)));
					field_exists = false;
					field_exists_or_not("EWayStatus", update_tblnm);
					if (!field_exists)
					{
						string text7 = "ALTER TABLE " + update_tblnm + " ADD COLUMN EWayStatus Text";
						cmd = new OleDbCommand(text7, con);
						cmd.ExecuteNonQuery();
					}
					string text8 = "update " + update_tblnm + " set ST38No='" + EWayBillNum + "',EWayStatus='Live' where voucherdate=#" + VoucherDate + "# and vouchernumber=" + voucherNumber + " and transtype='" + transtype + "'";
					cmd = new OleDbCommand(text8, con);
					cmd.ExecuteNonQuery();
					text8 = "Update TempPrintEWayTable set fieldvalue='" + EWayBillNum + "' where fieldnm='ewbNo'";
					cmd = new OleDbCommand(text8, con);
					cmd.ExecuteNonQuery();
					open_browser = false;
					GET_INV_PDF_USING_IRN();
					open_browser = false;
				}
				string value = Conversions.ToString((int)Interaction.MsgBox("Your E-Way Bill is Successfully Generated\r\n\r\nE-Way Bill No. = " + EWayBillNum + "\r\n\r\nDo You Want To Print E-Way Bill ?", MsgBoxStyle.YesNo | MsgBoxStyle.Question, title));
				if (Conversions.ToDouble(value) == 6.0)
				{
					open_browser = true;
					PRINT_EWAY_BILL();
				}
				else
				{
					open_browser = false;
					PRINT_EWAY_BILL();
				}
			}
			else if (Operators.CompareString(action, "reprinteway", TextCompare: false) == 0)
			{
				open_browser = true;
				PRINT_EWAY_BILL();
				Application.Exit();
			}
			else if (Operators.CompareString(action, "canceleway", TextCompare: false) == 0)
			{
				EWayBill_Cancel();
				Application.Exit();
			}
		}

		private void btnEWay_Click(object sender, EventArgs e)
		{
			//IL_0094: Unknown result type (might be due to invalid IL or missing references)
			//IL_009e: Expected Obj, but got Unknown
			//IL_013c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0146: Expected Obj, but got Unknown
			//IL_0439: Unknown result type (might be due to invalid IL or missing references)
			//IL_0443: Expected Obj, but got Unknown
			//IL_046f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0479: Expected Obj, but got Unknown
			//IL_03ab: Unknown result type (might be due to invalid IL or missing references)
			//IL_03b5: Expected Obj, but got Unknown
			conn(database_name);
			action = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyRequireAPI");
			if (Operators.CompareString(party_api_bal, "", TextCompare: false) == 0)
			{
				JSON_Generate();
				Write_Json_file(TextBox1.Text);
			}
			else if (Operators.CompareString(action, "ewaybyirn", TextCompare: false) == 0)
			{
				Application.DoEvents();
				string text = "SELECT * from TempEWayTableByIRN where ItemSrNo=0";
				da = new OleDbDataAdapter(text, con);
				ds = new DataSet();
				((DbDataAdapter)(object)da).Fill(ds);
				dt = ds.Tables[0];
				string text2 = "";
				if (dt.Rows.Count > 0)
				{
					GenerateEwayWithoutIrn();
					return;
				}
				EWAY_Json_Generate();
				string eWAY_JSON_STR = EWAY_JSON_STR;
				Application.DoEvents();
				Thread.Sleep(10000);
				text = "SELECT FieldValue from " + Module1.Main_TABLE + " WHERE Fieldnm='MyURL'";
				da = new OleDbDataAdapter(text, con);
				ds = new DataSet();
				((DbDataAdapter)(object)da).Fill(ds);
				dt = ds.Tables[0];
				if (dt.Rows.Count > 0)
				{
					URL1 = dt.Rows[0].ItemArray[0].ToString();
				}
				string uRL = URL1;
				Write_Json_file(eWAY_JSON_STR);
				upload_data_6(uRL, eWAY_JSON_STR);
				string text3 = TextBox2.Text;
				if (Operators.CompareString(text3, "", TextCompare: false) == 0)
				{
					Interaction.MsgBox("Response Blank Received", MsgBoxStyle.Critical, "Bahi-Khata");
					Application.Exit();
				}
				text3 = text3.Replace("[", "");
				text3 = text3.Replace("]", "");
				Write_Txt_file(text3);
				JObject jObject = JObject.Parse(text3);
				string text4 = jObject["Irn"].ToString();
				string text5 = jObject["EwbNo"].ToString();
				string text6 = jObject["EwbDt"].ToString();
				string text7 = jObject["ErrorMessage"].ToString();
				text7 = text7.Replace("\"", "");
				if (Operators.CompareString(text7, "", TextCompare: false) != 0)
				{
					Interaction.MsgBox("Error Occurred\r\n\r\n" + text7 + "\r\n\r\n" + text3, MsgBoxStyle.Critical, title);
					return;
				}
				DEDUCT_API();
				text4 = text4.Replace("\"", "");
				text5 = text5.Replace("\"", "");
				text6 = text6.Replace("\"", "");
				EWay_Nw_IRN = text4;
				EWayBillNum = text5;
				yr = Conversions.ToString(DateAndTime.Year(Conversions.ToDate(text6)));
				mnth = Conversions.ToString(DateAndTime.Month(Conversions.ToDate(text6)));
				field_exists = false;
				field_exists_or_not("EWayStatus", update_tblnm);
				if (!field_exists)
				{
					string text8 = "ALTER TABLE " + update_tblnm + " ADD COLUMN EWayStatus Text";
					cmd = new OleDbCommand(text8, con);
					cmd.ExecuteNonQuery();
				}
				string text9 = "update " + update_tblnm + " set ST38No='" + EWayBillNum + "',EWayStatus='Live' where voucherdate=#" + VoucherDate + "# and vouchernumber=" + voucherNumber + " and transtype='" + transtype + "'";
				cmd = new OleDbCommand(text9, con);
				cmd.ExecuteNonQuery();
				text9 = "Update TempPrintEWayTable set fieldvalue='" + EWayBillNum + "' where fieldnm='ewbNo'";
				cmd = new OleDbCommand(text9, con);
				cmd.ExecuteNonQuery();
				open_browser = false;
				GET_INV_PDF_USING_IRN();
				open_browser = false;
				string value = Conversions.ToString((int)Interaction.MsgBox("Your E-Way Bill is Successfully Generated\r\n\r\nE-Way Bill No. = " + EWayBillNum + "\r\n\r\nFrom E-Inv. IRN No. = " + EWay_Nw_IRN + "\r\n\r\nDo You Want To Print E-Way Bill ?", MsgBoxStyle.YesNo | MsgBoxStyle.Question, title));
				if (Conversions.ToDouble(value) == 6.0)
				{
					open_browser = true;
					PRINT_EWAY_BILL();
				}
				else
				{
					open_browser = false;
					PRINT_EWAY_BILL();
				}
			}
			else if (Operators.CompareString(action, "reprinteway", TextCompare: false) == 0)
			{
				open_browser = true;
				PRINT_EWAY_BILL();
				Application.Exit();
			}
			else if (Operators.CompareString(action, "canceleway", TextCompare: false) == 0)
			{
				EWayBill_Cancel();
				Application.Exit();
			}
		}

		private void EWayBill_Cancel()
		{
			//IL_000f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0019: Expected Obj, but got Unknown
			string text = "SELECT * from TempCancelEWayTable";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			checked
			{
				int num = dt.Rows.Count - 1;
				string text2 = default;
				for (int i = 0; i <= num; i++)
				{
					if (Operators.CompareString(Strings.Left(dt.Rows[i]["FieldNm"].ToString(), 2), "My", TextCompare: false) != 0)
					{
						string left = Strings.UCase(dt.Rows[i]["FieldNm"].ToString());
						text2 = ((i != 0) ? (text2 + ",\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\"") : (text2 + "\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\""));
						if (Operators.CompareString(left, Strings.UCase("EWBPassword"), TextCompare: false) == 0)
						{
							text2 = "{" + text2 + "}";
							text2 = "[" + text2 + "]";
							text2 = "\"Push_Data_List\":" + text2;
						}
					}
				}
				text2 = "{" + text2 + "}";
				string text3 = text2;
				URL1 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyURL");
				string uRL = URL1;
				Write_Json_file(text3);
				upload_data_8(uRL, text3);
			}
		}

		private void PRINT_EWAY_BILL()
		{
			//IL_000f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0019: Expected Obj, but got Unknown
			string text = "SELECT * from TempPrintEWayTable order by itemsrno";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			checked
			{
				int num = dt.Rows.Count - 1;
				string text3 = default;
				for (int i = 0; i <= num; i++)
				{
					if (Operators.CompareString(Strings.Left(dt.Rows[i]["FieldNm"].ToString(), 2), "My", TextCompare: false) != 0)
					{
						string text2 = Strings.UCase(dt.Rows[i]["FieldNm"].ToString());
						text3 = ((i != 0) ? (text3 + ",\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\"") : (text3 + "\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\""));
					}
				}
				text3 = "{" + text3 + "}";
				string text4 = text3;
				string text5 = get_single_value("FieldValue", "TempPrintEWayTable", "Fieldnm", "MyURL");
				string url = text5;
				Write_Json_file(text4);
				upload_data_7(url, text4);
			}
		}

		private void upload_data_5(string url, string args)
		{
			int try0001_dispatch = -1;
			int num2 = default;
			JObject jObject = default;
			string text2 = default;
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
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							jObject = JObject.Parse(text);
						}
						if (Operators.CompareString(jObject["success"].ToString(), "1", TextCompare: false) == 0)
						{
							text2 = jObject["data"].ToString();
							JObject jObject2 = JObject.Parse(text);
							text2 = "";
							IEnumerator<JToken> enumerator = ((IEnumerable<JToken>)jObject2["data"]).GetEnumerator();
							while (enumerator.MoveNext())
							{
								JToken current = enumerator.Current;
								string text3 = current.ToString();
								text3 = text3.Replace("\"", "");
								string[] array = text3.Split(new char[1] { ':' });
								text2 = text2 + array[0].ToString() + " = " + array[1].ToString() + "\r\n";
							}
							enumerator?.Dispose();
						}
						else if (Operators.CompareString(jObject["success"].ToString(), "0", TextCompare: false) == 0)
						{
							string text4 = jObject["msg"].ToString();
							text4 = text4.Replace("\"", "");
							Interaction.MsgBox("Ehhh !!!\r\n\r\n\r\n" + text4 + "\r\n\r\n" + args, MsgBoxStyle.Critical, "Check API");
						}
						Interaction.MsgBox(text2, MsgBoxStyle.OkOnly, "Your API Balance as on dated " + Conversions.ToString(DateAndTime.Today.Date));
						break;
					}
					case 556:
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
						Interaction.MsgBox("Error Occured IRN upload_data_5 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 556;
					continue;
				}
				throw ProjectData.CreateProjectError(-2146828237);
			}
			if (num != 0)
			{
				ProjectData.ClearProjectError();
			}
		}

		private void BtnCheckBal_Click(object sender, EventArgs e)
		{
			conn(database_name);
			string text = "68c37f4121e93dc4b49b31820dd5602d7";
			string text2 = get_single_value("FieldValue", "TempShowAPIBalTable", "Fieldnm", "MyYourID");
			string args = "{\"apikey\":\"" + text + "\",\"party_id\":\"" + text2 + "\"}";
			string url = "http://kamrasoftwares.com/epanel/web-service/index.php?service=checkBalance";
			upload_data_5(url, args);
		}

		private void BtnAPIBal_Click(object sender, EventArgs e)
		{
			conn(database_name);
			string text = "68c37f4121e93dc4b49b31820dd5602d7";
			string text2 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyYourID");
			string text3 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyRequireAPI");
			string text4 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "Gstin");
			if (Operators.CompareString(text3, "", TextCompare: false) == 0)
			{
				Interaction.MsgBox("No Data Found In Table\r\n Table : " + Module1.Main_TABLE, MsgBoxStyle.Critical, title);
				return;
			}
			if (text3.Contains("einvoice"))
			{
				text3 = "einvoice";
			}
			else if (text3.Contains("eway"))
			{
				text3 = "eway";
			}
			string args = ((Operators.CompareString(Strings.Left(text2, 1), "A", TextCompare: false) != 0) ? ("{\"apikey\":\"" + text + "\",\"party_id\":\"" + text2 + "\",\"api_parameter\":\"" + text3 + "\",\"gst\":\"" + text4 + "\"}") : ("{\"apikey\":\"" + text + "\",\"party_id\":\"" + text2 + "\",\"api_parameter\":\"" + text3 + "\"}"));
			string url = "http://kamrasoftwares.com/epanel/web-service/index.php?service=checkTransactionBalance";
			upload_data_3(url, args);
		}

		private void upload_data_6(string url, string args)
		{
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
						if (Operators.CompareString(Information.Err().Description, "", TextCompare: false) != 0)
						{
							Interaction.MsgBox("Error Occured IRN upload_data_6 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
						}
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							TextBox2.Text = text;
						}
						break;
					}
					case 253:
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
						Interaction.MsgBox("Error Occured IRN upload_data_6 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 253;
					continue;
				}
				throw ProjectData.CreateProjectError(-2146828237);
			}
			if (num != 0)
			{
				ProjectData.ClearProjectError();
			}
		}

		private void Distance_UPDATE()
		{
			action = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyRequireAPI");
			string text = "68c37f4121e93dc4b49b31820dd5602d7";
			string text2 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyYourID");
			string text3 = (Entered_distance = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "DistanceEntered"));
			string left = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyRequireAPI");
			string text4 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "SourcePincode");
			string text5 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "DestinationPincode");
			if (Operators.CompareString(left, "", TextCompare: false) == 0)
			{
				Interaction.MsgBox("No Data Found In Table\r\n Table : " + Module1.Main_TABLE, MsgBoxStyle.Critical, title);
				return;
			}
			string args = "{\"apikey\":\"" + text + "\",\"userid\":\"" + text2 + "\",\"SourcePincode\":\"" + text4 + "\",\"DestinationPincode\":\"" + text5 + "\",\"Distance\":\"" + text3 + "\"}";
			string url = "http://kamrasoftwares.com/epanel/web-service/index.php?service=updatedistance";
			upload_data_31(url, args);
		}

		private void Distance_API()
		{
			//IL_002f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0039: Expected Obj, but got Unknown
			if (Operators.CompareString(party_api_bal, "", TextCompare: false) == 0)
			{
				return;
			}
			string text = "SELECT * from TempDistance";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			string text2 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyURL");
			checked
			{
				int num = dt.Rows.Count - 1;
				string text3 = default;
				for (int i = 0; i <= num; i++)
				{
					if (Operators.CompareString(Strings.Left(dt.Rows[i]["FieldNm"].ToString(), 2), "My", TextCompare: false) != 0)
					{
						string left = Strings.UCase(dt.Rows[i]["FieldNm"].ToString());
						text3 = ((i != 0) ? (text3 + ",\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\"") : (text3 + "\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\""));
						if (Operators.CompareString(left, Strings.UCase("EWBPassword"), TextCompare: false) == 0)
						{
							text3 = text3 + ",\"apiurl\":\"" + text2 + "\"";
							text3 = "{" + text3 + "}";
							text3 = "[" + text3 + "]";
							text3 = "\"Push_Data_List\":" + text3;
						}
					}
				}
				text3 = "{" + text3 + "}";
				string text4 = text3;
				URL1 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyURL");
				string url = "http://kamrasoftwares.com/epanel/web-service/index.php?service=getdistance";
				Write_Json_file(text4);
				upload_data_11_New(url, text4);
			}
		}

		private void btnDistance_Click(object sender, EventArgs e)
		{
			conn(database_name);
			Distance_API();
		}

		private void upload_data_7(string url, string args)
		{
			//IL_0214: Unknown result type (might be due to invalid IL or missing references)
			//IL_021e: Expected Obj, but got Unknown
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
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						text = text.Replace("[", "");
						text = text.Replace("]", "");
						Write_Txt_file(text);
						PDF_File = "";
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							PDF_File = text;
							PDF_File = PDF_File.Replace("\"", "");
							if (Operators.CompareString(PDF_File, "", TextCompare: false) == 0)
							{
								Interaction.MsgBox("Error Occured\r\n\r\nPDF File Not Generated", MsgBoxStyle.Critical, title);
								goto end_IL_0001;
							}
							string fileName = Application.StartupPath + "\\~temp~.pdf";
							WebClient webClient = new WebClient();
							webClient.DownloadFile(PDF_File, fileName);
							string text2 = ConvertFileToBase64(fileName);
							if (Operators.CompareString(update_tblnm, "", TextCompare: false) != 0)
							{
								string text3 = ((Operators.CompareString(transtype, "Jrnl", TextCompare: false) != 0) ? ("Update " + update_tblnm + " set EInvPDF='" + text2 + "' WHERE VoucherDate=#" + VoucherDate + "# and VoucherNumber=" + voucherNumber + " and TransType='" + transtype + "'") : (("Update " + update_tblnm + " set EInvPDF='" + text2 + "' WHERE VoucherDate=#" + VoucherDate + "# and VoucherNumber=" + voucherNumber) ?? ""));
								cmd = new OleDbCommand(text3, con);
								cmd.Parameters.Add("@img", (object)SqlDbType.Image).Value = bytes;
								cmd.ExecuteNonQuery();
							}
							if (open_browser)
							{
								string pDF_File = PDF_File;
								Process.Start(pDF_File);
							}
						}
						DEDUCT_API();
						break;
					}
					case 712:
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
						Interaction.MsgBox("Error Occured IRN upload_data_7 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001_2:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 712;
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

		private void upload_data_8(string url, string args)
		{
			//IL_01e3: Unknown result type (might be due to invalid IL or missing references)
			//IL_01ed: Expected Obj, but got Unknown
			//IL_0168: Unknown result type (might be due to invalid IL or missing references)
			//IL_0172: Expected Obj, but got Unknown
			int try0001_dispatch = -1;
			int num2 = default;
			string text3 = default;
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
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						text = text.Replace("[", "");
						text = text.Replace("]", "");
						text = text.Replace("\\", "");
						text = Strings.Mid(text, 2, checked(text.Length - 2));
						JObject jObject = JObject.Parse(text);
						Write_Txt_file(text);
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							string text2 = jObject["ErrorMessage"].ToString();
							text2 = text2.Replace("\"", "");
							if (Operators.CompareString(text2, "", TextCompare: false) != 0)
							{
								Interaction.MsgBox("Error Occurred\r\n\r\n" + text2 + "\r\n\r\n" + text, MsgBoxStyle.Critical, title);
								goto end_IL_0001;
							}
							text3 = jObject["EWayBill"].ToString();
						}
						field_exists = false;
						field_exists_or_not("EWayStatus", update_tblnm);
						if (!field_exists)
						{
							string text4 = "ALTER TABLE " + update_tblnm + " ADD COLUMN EWayStatus Text";
							cmd = new OleDbCommand(text4, con);
							cmd.ExecuteNonQuery();
						}
						string text5 = "update " + update_tblnm + " set EWayStatus='Cancel' where voucherdate=#" + VoucherDate + "# and vouchernumber=" + voucherNumber + " and transtype='" + transtype + "'";
						cmd = new OleDbCommand(text5, con);
						cmd.ExecuteNonQuery();
						DEDUCT_API();
						Interaction.MsgBox("Your E-way Bill No. : " + text3 + " has been Successfully Cancelled", MsgBoxStyle.Information, title);
						break;
					}
					case 626:
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
						Interaction.MsgBox("Error Occured IRN upload_data_8 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001_2:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 626;
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

		private void upload_data_10(string url, string args)
		{
			//IL_013e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0148: Expected Obj, but got Unknown
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
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						text = text.Replace("[", "");
						text = text.Replace("]", "");
						text = text.Replace("\\", "");
						text = Strings.Mid(text, 2, checked(text.Length - 2));
						JObject jObject = JObject.Parse(text);
						Write_Txt_file(text);
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							string text2 = jObject["ErrorMessage"].ToString();
							text2 = text2.Replace("\"", "");
							if (Operators.CompareString(text2, "", TextCompare: false) != 0)
							{
								Interaction.MsgBox("Error Occurred\r\n\r\n" + text2 + "\r\n\r\n" + text, MsgBoxStyle.Critical, title);
								goto end_IL_0001;
							}
							string text3 = jObject["Distance"].ToString();
							string text4 = "Insert into TempDistance(ItemSrNo,FieldNm,FieldValue) values(1,'Distance','" + text3 + "')";
							cmd = new OleDbCommand(text4, con);
							cmd.ExecuteNonQuery();
							DEDUCT_API();
						}
						break;
					}
					case 431:
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
						Interaction.MsgBox("Error Occured IRN upload_data_10 :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
					}
					break;
					end_IL_0001_2:;
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 431;
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

		private void NewApis_Click(object sender, EventArgs e)
		{
			conn(database_name);
			JSON_Generate_New_API_GET_GSN_Details();
			string text = TextBox1.Text;
			URL1 = get_single_value("FieldValue", Module1.Main_TABLE, "Fieldnm", "MyURL");
			string uRL = URL1;
			Write_Json_file(text);
			upload_data_12(uRL, text);
			string text2 = TextBox2.Text;
			text2 = text2.Replace("[", "");
			text2 = text2.Replace("]", "");
			Write_Txt_file(text2);
			JObject jObject = JObject.Parse(text2);
			string text3 = jObject["EWBDetails"]["gstin"].ToString();
			string text4 = jObject["EWBDetails"]["tradeName"].ToString();
			string text5 = jObject["EWBDetails"]["legalName"].ToString();
			string text6 = jObject["EWBDetails"]["address1"].ToString();
			string text7 = jObject["EWBDetails"]["address2"].ToString();
			string text8 = jObject["EWBDetails"]["stateCode"].ToString();
			string text9 = jObject["EWBDetails"]["pinCode"].ToString();
			string text10 = jObject["EWBDetails"]["txpType"].ToString();
			string text11 = jObject["EWBDetails"]["status"].ToString();
			string text12 = jObject["EWBDetails"]["resstatus"].ToString();
			string text13 = jObject["EWBDetails"]["blkStatus"].ToString();
			string text14 = "GSTIN : " + text3 + "\r\n\r\n\r\nTrade Name : " + text4 + "\r\n\r\nLegal Name : " + text5 + "\r\n\r\nAddress : " + text6 + "," + text7 + "\r\n\r\n\r\nPinCode : " + text9 + "\r\n\r\nTaxPayer Type : " + text10 + "\r\n\r\nStatus : " + text11;
			text14 = ((!((Operators.CompareString(text5, "null", TextCompare: false) == 0) | (Operators.CompareString(text5, "Null", TextCompare: false) == 0))) ? ("GSTIN : " + text3 + "\r\n\r\n\r\nTrade Name : " + text4 + "\r\n\r\nLegal Name : " + text5 + "\r\n\r\nAddress : " + text6 + "," + text7 + "\r\n\r\n\r\nPinCode : " + text9 + "\r\n\r\nTaxPayer Type : " + text10 + "\r\n\r\nStatus : " + text11) : ("GSTIN : " + text3 + "\r\n\r\n\r\nTrade Name : " + text4 + "\r\n\r\nAddress : " + text6 + "," + text7 + "\r\n\r\n\r\nPinCode : " + text9 + "\r\n\r\nTaxPayer Type : " + text10 + "\r\n\r\nStatus : " + text11));
			text14 = text14.Replace("\"", "");
			text14 = text14.Replace("null", "");
			text14 = text14.Replace("Null", "");
			text14 = text14.Replace("ACT", "Active");
			text14 = text14.Replace("REG", "Regular");
			text14 = text14.Replace("SUS", "Suspended");
			Interaction.MsgBox(text14, MsgBoxStyle.Information, "Search GSTIN Info. By Bahi-Khata");
			DEDUCT_API();
		}

		private void New_Json_file_create(string head_field)
		{
			//IL_001a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0024: Expected Obj, but got Unknown
			string text = "SELECT FieldValue,FieldHeading_Jsn,FieldNm_Jsn from TempEInvoiceTable WHERE FieldHeading_Jsn<>'' AND FieldHeading_Jsn='" + head_field + "' and itemsrno=1";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			if (ds.Tables[0].Rows.Count <= 0)
			{
				return;
			}
			checked
			{
				int num = dt.Rows.Count - 1;
				for (int i = 0; i <= num; i++)
				{
					string text2 = dt.Rows[i]["FieldValue"].ToString();
					if (Operators.CompareString(text2, "", TextCompare: false) == 0)
					{
						text2 = "null";
					}
					string text3 = dt.Rows[i]["FieldHeading_Jsn"].ToString();
					if (i == 0)
					{
						json_String = json_String + "\"" + text3 + "\":{";
						if (Operators.CompareString(head_field, "TranDtls", TextCompare: false) == 0)
						{
							json_String += "\"TaxSch\":\"GST\",";
						}
						json_String = json_String + "\"" + dt.Rows[i]["FieldNm_Jsn"].ToString() + "\":\"" + text2 + "\",";
					}
					else if (i < dt.Rows.Count - 1)
					{
						json_String = json_String + "\"" + dt.Rows[i]["FieldNm_Jsn"].ToString() + "\":\"" + text2 + "\",";
					}
					else
					{
						json_String = (json_String + "\"" + dt.Rows[i]["FieldNm_Jsn"].ToString() + "\":\"" + text2) ?? "";
					}
				}
				json_String += "\"},";
			}
		}

		private void BtnSave_Click(object sender, EventArgs e)
		{
			//IL_0070: Unknown result type (might be due to invalid IL or missing references)
			//IL_007a: Expected Obj, but got Unknown
			conn(database_name);
			New_Json_file_create("TranDtls");
			New_Json_file_create("DocDtls");
			New_Json_file_create("SellerDtls");
			New_Json_file_create("BuyerDtls");
			New_Json_file_create("DispDtls");
			New_Json_file_create("ValDtls");
			New_Json_file_create("EwbDtls");
			string text = "SELECT ItemSrNo, FieldValue,FieldHeading_Jsn,FieldNm_Jsn from TempEInvoiceTable WHERE FieldHeading_Jsn<>'' AND FieldHeading_Jsn='ItemList' order by ItemSrNo";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			checked
			{
				if (ds.Tables[0].Rows.Count > 0)
				{
					int num = dt.Rows.Count - 1;
					for (int i = 0; i <= num; i++)
					{
						string text2 = dt.Rows[i]["FieldValue"].ToString();
						if (Operators.CompareString(text2, "", TextCompare: false) == 0)
						{
							text2 = "null";
						}
						string text3 = dt.Rows[i]["FieldHeading_Jsn"].ToString();
						if (i == 0)
						{
							json_String = json_String + "\"" + text3 + "\":[{";
							json_String = json_String + "\"" + dt.Rows[i]["FieldNm_Jsn"].ToString() + "\":\"" + text2 + "\",";
						}
						else if (i < dt.Rows.Count - 1)
						{
							if (i < dt.Rows.Count && Operators.CompareString(dt.Rows[i]["ItemSrNo"].ToString(), dt.Rows[i + 1]["ItemSrNo"].ToString(), TextCompare: false) != 0)
							{
								json_String = Strings.Mid(json_String, 1, json_String.Length - 1);
								json_String += "},{";
							}
							json_String = json_String + "\"" + dt.Rows[i]["FieldNm_Jsn"].ToString() + "\":\"" + text2 + "\",";
						}
						else
						{
							if (i > dt.Rows.Count && Operators.CompareString(dt.Rows[i]["ItemSrNo"].ToString(), dt.Rows[i + 1]["ItemSrNo"].ToString(), TextCompare: false) == 0)
							{
								json_String = json_String + "\"" + text3 + "\":{";
							}
							json_String = (json_String + "\"" + dt.Rows[i]["FieldNm_Jsn"].ToString() + "\":\"" + text2) ?? "";
						}
					}
					json_String += "\"}]";
				}
				string text4 = "Version";
				string text5 = "1.1";
				json_String = "[{\"" + text4 + "\":\"" + text5 + "\"," + json_String + "}]";
				json_String = json_String.Replace("\"null\"", "null");
				Interaction.MsgBox(json_String);
				Write_Json_file(json_String);
			}
		}

		private void upload_data_11_New(string url, string args)
		{
			//IL_00bf: Unknown result type (might be due to invalid IL or missing references)
			//IL_00c9: Expected Obj, but got Unknown
			int try0001_dispatch = -1;
			int num2 = default;
			JObject jObject = default;
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
						Uri uri = new Uri(url);
						byte[] bytes = Encoding.UTF8.GetBytes(args);
						string text = SendRequest(uri, bytes, "application/json", "POST");
						if (Operators.CompareString(text, "", TextCompare: false) != 0)
						{
							jObject = JObject.Parse(text);
						}
						if (Operators.CompareString(jObject["success"].ToString(), "1", TextCompare: false) == 0)
						{
							string text2 = jObject["Distance"].ToString();
							text2 = text2.Replace("\"", "");
							string text3 = "Insert into TempDistance(ItemSrNo,FieldNm,FieldValue) values(1,'Distance','" + text2 + "')";
							cmd = new OleDbCommand(text3, con);
							cmd.ExecuteNonQuery();
							new_param = jObject["newapiparamete"].ToString();
							new_param = new_param.Replace("\"", "");
							DEDUCT_API();
						}
						else
						{
							string text4 = jObject["ErrorMessage"].ToString();
							text4 = text4.Replace("\"", "");
							if (Operators.CompareString(text4, "", TextCompare: false) != 0)
							{
								Interaction.MsgBox("Ehhh !!!\r\n\r\n\r\n" + text4 + "\r\n\r\n" + args, MsgBoxStyle.Critical, "Check API");
							}
						}
						goto end_IL_0001;
					}
					case 454:
						num = -1;
						switch (num2)
						{
						case 2:
							if (Operators.CompareString(Information.Err().Description, "", TextCompare: false) != 0)
							{
								Interaction.MsgBox("Error Occured IRN upload_data_11_New :\r\n" + Information.Err().Description + "\r\n" + Conversions.ToString(Information.Err().Number));
							}
							goto end_IL_0001;
						}
						break;
					}
				}
				catch (Exception ex) when ((num2 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 454;
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

		private void EWAYWithoutIRNJsonGenerate(string TrailString)
		{
			//IL_000f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0019: Expected Obj, but got Unknown
			string text = "SELECT * from TempEWayTableByIRN order by itemsrno";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			string text2 = "";
			checked
			{
				int num = dt.Rows.Count - 1;
				for (int i = 0; i <= num; i++)
				{
					if (Operators.CompareString(Strings.Left(dt.Rows[i]["FieldNm"].ToString(), 2), "My", TextCompare: false) != 0 && Conversions.ToDouble(dt.Rows[i]["ItemSrNo"].ToString()) > 0.0)
					{
						string text3 = Strings.UCase(dt.Rows[i]["FieldNm"].ToString());
						if ((Conversions.ToDouble(dt.Rows[i - 1]["ItemSrNo"].ToString()) > 0.0) & (Operators.CompareString(dt.Rows[i - 1]["ItemSrNo"].ToString(), dt.Rows[i]["ItemSrNo"].ToString(), TextCompare: false) != 0))
						{
							text2 += "},{";
						}
						if ((Operators.CompareString(text2, "", TextCompare: false) == 0) | (Operators.CompareString(text2, null, TextCompare: false) == 0))
						{
							text2 = text2 + "\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\"";
						}
						else
						{
							text2 = ((Operators.CompareString(Conversions.ToString(text2[text2.Length - 1]), "{", TextCompare: false) != 0) ? (text2 + ",\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\"") : (text2 + "\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\""));
						}
					}
				}
				text2 = "{" + text2 + "}";
				text2 = "[" + text2 + "]";
				text2 = text2 + "," + TrailString;
				text2 = "\"Push_Data_List\":" + text2;
				text2 = "{" + text2 + "}";
				EWAY_JSON_STR = text2;
			}
		}

		private void EWAY_Json_Generate()
		{
			//IL_000f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0019: Expected Obj, but got Unknown
			string text = "SELECT * from TempEWayTableByIRN order by itemsrno";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			checked
			{
				int num = dt.Rows.Count - 1;
				string text3 = default;
				for (int i = 0; i <= num; i++)
				{
					if (Operators.CompareString(Strings.Left(dt.Rows[i]["FieldNm"].ToString(), 2), "My", TextCompare: false) != 0)
					{
						string text2 = Strings.UCase(dt.Rows[i]["FieldNm"].ToString());
						text3 = ((i != 0) ? (text3 + ",\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\"") : (text3 + "\"" + dt.Rows[i]["FieldNm"].ToString() + "\":\"" + dt.Rows[i]["FieldValue"].ToString() + "\""));
					}
				}
				text3 = "{" + text3 + "}";
				text3 = "[" + text3 + "]";
				text3 = "\"Push_Data_List\":" + text3;
				text3 = "{" + text3 + "}";
				EWAY_JSON_STR = text3;
			}
		}

		private string SendRequest(Uri uri, byte[] jsonDataBytes, string contentType, string method)
		{
			ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
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

		public string GenerateEinvoiceQRCode(string QRCode, string type)
		{
			//IL_0034: Unknown result type (might be due to invalid IL or missing references)
			//IL_003b: Expected Obj, but got Unknown
			string empty = string.Empty;
			try
			{
				string empty2 = string.Empty;
				MemoryStream memoryStream = new MemoryStream();
				BarcodeWriter barcodeWriter = new BarcodeWriter();
				barcodeWriter.Format = BarcodeFormat.QR_CODE;
				Bitmap val = barcodeWriter.Write(QRCode);
				Bitmap val2 = new Bitmap((Image)(object)val);
				((Image)val2).Save((Stream)memoryStream, ImageFormat.Jpeg);
				byte[] array = memoryStream.ToArray();
				Image val3 = Image.FromStream((Stream)memoryStream);
				string text = Application.StartupPath + "\\qrimg\\";
				try
				{
					if (!Directory.Exists(text))
					{
						DirectoryInfo directoryInfo = Directory.CreateDirectory(text);
					}
				}
				catch (IOException ex)
				{
					ProjectData.SetProjectError(ex);
					IOException ex2 = ex;
					ProjectData.ClearProjectError();
				}
				Regex regex = new Regex("[*'\",_&#^@]");
				empty2 = DateTime.Now.ToFileTime() + ".bmp";
				string text2 = text + empty2;
				if (!File.Exists(text2) && 0 == 0)
				{
					val3.Save(text2, ImageFormat.Bmp);
					if (Operators.CompareString(type, "QR", TextCompare: false) == 0)
					{
						PictureBox1.Image = Image.FromFile(text2);
					}
					if (Operators.CompareString(type, "INV", TextCompare: false) == 0)
					{
						PictureBox2.Image = Image.FromFile(text2);
					}
				}
			}
			catch (Exception ex3)
			{
				ProjectData.SetProjectError(ex3);
				Exception ex4 = ex3;
				ProjectData.ClearProjectError();
			}
			return empty;
		}

		private void Write_IRN(string data)
		{
			string path = Application.StartupPath + "\\IRNNO.txt";
			if (!File.Exists(path))
			{
				File.Create(path).Dispose();
			}
			using StreamWriter streamWriter = new StreamWriter(path, append: true);
			streamWriter.WriteLine("a" + data);
		}

		private void Write_Txt_file(string data)
		{
			string path = Application.StartupPath + "\\resp.txt";
			StreamWriter streamWriter = new StreamWriter(path);
			streamWriter.Write(data);
			streamWriter.Close();
		}

		private void Write_Json_file(string data)
		{
			string path = Application.StartupPath + "\\zn";
			StreamWriter streamWriter = new StreamWriter(path);
			streamWriter.Write(data);
			streamWriter.Close();
		}

		private void JSON_Generate()
		{
			//IL_001c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0026: Expected Obj, but got Unknown
			//IL_0341: Unknown result type (might be due to invalid IL or missing references)
			//IL_034b: Expected Obj, but got Unknown
			//IL_00e3: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ed: Expected Obj, but got Unknown
			conn(database_name);
			string text = "SELECT distinct(ItemSrNo) from TempEInvoiceTable where ItemSrNo<>11111 order by itemsrno";
			da1 = new OleDbDataAdapter(text, con);
			ds1 = new DataSet();
			((DbDataAdapter)(object)da1).Fill(ds1);
			dt1 = ds1.Tables[0];
			checked
			{
				string[] array = new string[dt1.Rows.Count - 1 + 1];
				string text2 = default;
				if (dt1.Rows.Count > 1)
				{
					int num = dt1.Rows.Count - 1;
					for (int i = 0; i <= num; i++)
					{
						text = "SELECT * from TempEInvoiceTable where ItemSrNo=" + dt1.Rows[i]["ItemSrNo"].ToString() + " order by itemsrno";
						da = new OleDbDataAdapter(text, con);
						ds = new DataSet();
						((DbDataAdapter)(object)da).Fill(ds);
						dt = ds.Tables[0];
						int num2 = dt.Rows.Count - 1;
						for (int j = 0; j <= num2; j++)
						{
							if (Operators.CompareString(Strings.Left(dt.Rows[j]["FieldNm"].ToString(), 2), "My", TextCompare: false) != 0)
							{
								text2 = ((j != 0) ? (text2 + ",\"" + dt.Rows[j]["FieldNm"].ToString() + "\":\"" + dt.Rows[j]["FieldValue"].ToString() + "\"") : (text2 + "\"" + dt.Rows[j]["FieldNm"].ToString() + "\":\"" + dt.Rows[j]["FieldValue"].ToString() + "\""));
							}
						}
						text2 = "{" + text2 + "}";
						text2 = "\"Data\":[" + text2 + "]";
						array[i] = text2;
						text2 = "";
					}
					int num3 = array.Length - 1;
					for (int k = 0; k <= num3; k++)
					{
						text2 = ((k != 0) ? (text2 + "," + array[k]) : (text2 + array[k]));
					}
					text2 = "\"Push_Data_List\":{" + text2 + "}";
					text2 = "{" + text2 + "}";
					TextBox1.Text = text2;
					return;
				}
				text = "SELECT * from TempEInvoiceTable order by itemsrno";
				da = new OleDbDataAdapter(text, con);
				ds = new DataSet();
				((DbDataAdapter)(object)da).Fill(ds);
				dt = ds.Tables[0];
				int num4 = dt.Rows.Count - 1;
				for (int l = 0; l <= num4; l++)
				{
					if (Operators.CompareString(Strings.Left(dt.Rows[l]["FieldNm"].ToString(), 2), "My", TextCompare: false) != 0)
					{
						text2 = ((l != 0) ? (text2 + ",\"" + dt.Rows[l]["FieldNm"].ToString() + "\":\"" + dt.Rows[l]["FieldValue"].ToString() + "\"") : (text2 + "\"" + dt.Rows[l]["FieldNm"].ToString() + "\":\"" + dt.Rows[l]["FieldValue"].ToString() + "\""));
					}
				}
				text2 = "{" + text2 + "}";
				text2 = "\"Data\":[" + text2 + "]";
				text2 = "\"Push_Data_List\":{" + text2 + "}";
				text2 = "{" + text2 + "}";
				TextBox1.Text = text2;
			}
		}

		private void JSON_Generate_New_API_GET_GSN_Details()
		{
			//IL_001c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0026: Expected Obj, but got Unknown
			//IL_0341: Unknown result type (might be due to invalid IL or missing references)
			//IL_034b: Expected Obj, but got Unknown
			//IL_00e3: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ed: Expected Obj, but got Unknown
			conn(database_name);
			string text = "SELECT distinct(ItemSrNo) from TempGSTNstatus where ItemSrNo<>11111 order by itemsrno";
			da1 = new OleDbDataAdapter(text, con);
			ds1 = new DataSet();
			((DbDataAdapter)(object)da1).Fill(ds1);
			dt1 = ds1.Tables[0];
			checked
			{
				string[] array = new string[dt1.Rows.Count - 1 + 1];
				string text2 = default;
				if (dt1.Rows.Count > 1)
				{
					int num = dt1.Rows.Count - 1;
					for (int i = 0; i <= num; i++)
					{
						text = "SELECT * from TempGSTNstatus where ItemSrNo=" + dt1.Rows[i]["ItemSrNo"].ToString() + " order by itemsrno";
						da = new OleDbDataAdapter(text, con);
						ds = new DataSet();
						((DbDataAdapter)(object)da).Fill(ds);
						dt = ds.Tables[0];
						int num2 = dt.Rows.Count - 1;
						for (int j = 0; j <= num2; j++)
						{
							if (Operators.CompareString(Strings.Left(dt.Rows[j]["FieldNm"].ToString(), 2), "My", TextCompare: false) != 0)
							{
								text2 = ((j != 0) ? (text2 + ",\"" + dt.Rows[j]["FieldNm"].ToString() + "\":\"" + dt.Rows[j]["FieldValue"].ToString() + "\"") : (text2 + "\"" + dt.Rows[j]["FieldNm"].ToString() + "\":\"" + dt.Rows[j]["FieldValue"].ToString() + "\""));
							}
						}
						text2 = "{" + text2 + "}";
						text2 = "\"Data\":[" + text2 + "]";
						array[i] = text2;
						text2 = "";
					}
					int num3 = array.Length - 1;
					for (int k = 0; k <= num3; k++)
					{
						text2 = ((k != 0) ? (text2 + "," + array[k]) : (text2 + array[k]));
					}
					text2 = "\"Push_Data_List\":{" + text2 + "}";
					text2 = "{" + text2 + "}";
					TextBox1.Text = text2;
					return;
				}
				text = "SELECT * from TempGSTNstatus order by itemsrno";
				da = new OleDbDataAdapter(text, con);
				ds = new DataSet();
				((DbDataAdapter)(object)da).Fill(ds);
				dt = ds.Tables[0];
				int num4 = dt.Rows.Count - 1;
				for (int l = 0; l <= num4; l++)
				{
					if (Operators.CompareString(Strings.Left(dt.Rows[l]["FieldNm"].ToString(), 2), "My", TextCompare: false) != 0)
					{
						text2 = ((l != 0) ? (text2 + ",\"" + dt.Rows[l]["FieldNm"].ToString() + "\":\"" + dt.Rows[l]["FieldValue"].ToString() + "\"") : (text2 + "\"" + dt.Rows[l]["FieldNm"].ToString() + "\":\"" + dt.Rows[l]["FieldValue"].ToString() + "\""));
					}
				}
				text2 = "{" + text2 + "}";
				TextBox1.Text = text2;
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
			//IL_004a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0054: Expected Obj, but got Unknown
			//IL_0056: Unknown result type (might be due to invalid IL or missing references)
			//IL_0060: Expected Obj, but got Unknown
			//IL_0062: Unknown result type (might be due to invalid IL or missing references)
			//IL_006c: Expected Obj, but got Unknown
			//IL_006e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0078: Expected Obj, but got Unknown
			//IL_007a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0084: Expected Obj, but got Unknown
			//IL_0271: Unknown result type (might be due to invalid IL or missing references)
			//IL_027b: Expected Obj, but got Unknown
			//IL_02fa: Unknown result type (might be due to invalid IL or missing references)
			//IL_0304: Expected Obj, but got Unknown
			//IL_0383: Unknown result type (might be due to invalid IL or missing references)
			//IL_038d: Expected Obj, but got Unknown
			//IL_040c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0416: Expected Obj, but got Unknown
			//IL_0491: Unknown result type (might be due to invalid IL or missing references)
			//IL_049b: Expected Obj, but got Unknown
			//IL_051a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0524: Expected Obj, but got Unknown
			//IL_05a5: Unknown result type (might be due to invalid IL or missing references)
			//IL_05af: Expected Obj, but got Unknown
			PictureBox2 = new PictureBox();
			TextBox2 = new TextBox();
			TextBox1 = new TextBox();
			PictureBox1 = new PictureBox();
			BtnAPIBal = new Button();
			BtnCheckBal = new Button();
			btnEWay = new Button();
			btnEInvoice = new Button();
			btnDistance = new Button();
			searchGSTIN = new Button();
			BtnSave = new Button();
			((ISupportInitialize)PictureBox2).BeginInit();
			((ISupportInitialize)PictureBox1).BeginInit();
			((Control)this).SuspendLayout();
			((Control)PictureBox2).Location = new Point(362, 321);
			((Control)PictureBox2).Name = "PictureBox2";
			((Control)PictureBox2).Size = new Size(189, 184);
			PictureBox2.TabIndex = 15;
			PictureBox2.TabStop = false;
			((Control)PictureBox2).Visible = false;
			((Control)TextBox2).Location = new Point(890, 313);
			TextBox2.Multiline = true;
			((Control)TextBox2).Name = "TextBox2";
			((Control)TextBox2).Size = new Size(266, 192);
			((Control)TextBox2).TabIndex = 14;
			((Control)TextBox2).Visible = false;
			((Control)TextBox1).Location = new Point(604, 313);
			TextBox1.Multiline = true;
			((Control)TextBox1).Name = "TextBox1";
			((Control)TextBox1).Size = new Size(266, 192);
			((Control)TextBox1).TabIndex = 13;
			((Control)TextBox1).Visible = false;
			((Control)PictureBox1).Location = new Point(167, 321);
			((Control)PictureBox1).Name = "PictureBox1";
			((Control)PictureBox1).Size = new Size(189, 184);
			PictureBox1.TabIndex = 12;
			PictureBox1.TabStop = false;
			((Control)PictureBox1).Visible = false;
			((Control)BtnAPIBal).Font = new Font("Segoe UI", 11f, (FontStyle)1);
			((Control)BtnAPIBal).Location = new Point(658, 92);
			((Control)BtnAPIBal).Name = "BtnAPIBal";
			((Control)BtnAPIBal).Size = new Size(171, 69);
			((Control)BtnAPIBal).TabIndex = 11;
			((ButtonBase)BtnAPIBal).Text = "API Bal";
			((ButtonBase)BtnAPIBal).UseVisualStyleBackColor = true;
			((Control)BtnCheckBal).Font = new Font("Segoe UI", 11f, (FontStyle)1);
			((Control)BtnCheckBal).Location = new Point(481, 92);
			((Control)BtnCheckBal).Name = "BtnCheckBal";
			((Control)BtnCheckBal).Size = new Size(171, 69);
			((Control)BtnCheckBal).TabIndex = 10;
			((ButtonBase)BtnCheckBal).Text = "Check Bal";
			((ButtonBase)BtnCheckBal).UseVisualStyleBackColor = true;
			((Control)btnEWay).Font = new Font("Segoe UI", 11f, (FontStyle)1);
			((Control)btnEWay).Location = new Point(304, 92);
			((Control)btnEWay).Name = "btnEWay";
			((Control)btnEWay).Size = new Size(171, 69);
			((Control)btnEWay).TabIndex = 9;
			((ButtonBase)btnEWay).Text = "Eway";
			((ButtonBase)btnEWay).UseVisualStyleBackColor = true;
			((Control)btnEInvoice).Font = new Font("Segoe UI", 11f, (FontStyle)1);
			((Control)btnEInvoice).Location = new Point(127, 92);
			((Control)btnEInvoice).Name = "btnEInvoice";
			((Control)btnEInvoice).Size = new Size(171, 69);
			((Control)btnEInvoice).TabIndex = 8;
			((ButtonBase)btnEInvoice).Text = "EInvoice";
			((ButtonBase)btnEInvoice).UseVisualStyleBackColor = true;
			((Control)btnDistance).Font = new Font("Segoe UI", 11f, (FontStyle)1);
			((Control)btnDistance).Location = new Point(1012, 92);
			((Control)btnDistance).Name = "btnDistance";
			((Control)btnDistance).Size = new Size(171, 69);
			((Control)btnDistance).TabIndex = 16;
			((ButtonBase)btnDistance).Text = "Distance";
			((ButtonBase)btnDistance).UseVisualStyleBackColor = true;
			((Control)searchGSTIN).Font = new Font("Segoe UI", 11f, (FontStyle)1);
			((Control)searchGSTIN).Location = new Point(835, 92);
			((Control)searchGSTIN).Name = "searchGSTIN";
			((Control)searchGSTIN).Size = new Size(171, 69);
			((Control)searchGSTIN).TabIndex = 17;
			((ButtonBase)searchGSTIN).Text = "Search GSTIN";
			((ButtonBase)searchGSTIN).UseVisualStyleBackColor = true;
			((Control)BtnSave).Font = new Font("Microsoft Sans Serif", 12f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)BtnSave).Location = new Point(1057, 184);
			((Control)BtnSave).Name = "BtnSave";
			((Control)BtnSave).Size = new Size(126, 52);
			((Control)BtnSave).TabIndex = 34;
			((ButtonBase)BtnSave).Text = "Gen Json";
			((ButtonBase)BtnSave).UseVisualStyleBackColor = true;
			((ContainerControl)this).AutoScaleDimensions = new SizeF(6f, 13f);
			((ContainerControl)this).AutoScaleMode = (AutoScaleMode)1;
			((Form)this).ClientSize = new Size(1324, 270);
			((Control)this).Controls.Add((Control)(object)BtnSave);
			((Control)this).Controls.Add((Control)(object)searchGSTIN);
			((Control)this).Controls.Add((Control)(object)btnDistance);
			((Control)this).Controls.Add((Control)(object)PictureBox2);
			((Control)this).Controls.Add((Control)(object)TextBox2);
			((Control)this).Controls.Add((Control)(object)TextBox1);
			((Control)this).Controls.Add((Control)(object)PictureBox1);
			((Control)this).Controls.Add((Control)(object)BtnAPIBal);
			((Control)this).Controls.Add((Control)(object)BtnCheckBal);
			((Control)this).Controls.Add((Control)(object)btnEWay);
			((Control)this).Controls.Add((Control)(object)btnEInvoice);
			((Form)this).MaximizeBox = false;
			((Form)this).MinimizeBox = false;
			((Control)this).Name = "Form1";
			((Form)this).StartPosition = (FormStartPosition)1;
			((Form)this).Text = "CONNECTIVITY DASH-BOARD BY KAMRA SOFTWARES";
			((ISupportInitialize)PictureBox2).EndInit();
			((ISupportInitialize)PictureBox1).EndInit();
			((Control)this).ResumeLayout(false);
			((Control)this).PerformLayout();
		}
	}
	[DesignerGenerated]
	public class Form2 : Form
	{
		private IContainer components;

		private string load_pdf_path;

		[field: AccessedThroughProperty("SaveFileDialog1")]
		internal virtual SaveFileDialog SaveFileDialog1
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

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

		[field: AccessedThroughProperty("AxAcroPDF1")]
		internal virtual AxAcroPDF AxAcroPDF1
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

		public Form2()
		{
			((Form)this).Load += Form2_Load;
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
			//IL_0012: Unknown result type (might be due to invalid IL or missing references)
			//IL_001c: Expected Obj, but got Unknown
			//IL_001e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0028: Expected Obj, but got Unknown
			//IL_002a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0034: Expected Obj, but got Unknown
			//IL_0036: Unknown result type (might be due to invalid IL or missing references)
			//IL_0040: Expected Obj, but got Unknown
			//IL_004e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0058: Expected Obj, but got Unknown
			//IL_009f: Unknown result type (might be due to invalid IL or missing references)
			//IL_00a9: Expected Obj, but got Unknown
			//IL_0146: Unknown result type (might be due to invalid IL or missing references)
			//IL_0150: Expected Obj, but got Unknown
			//IL_01ed: Unknown result type (might be due to invalid IL or missing references)
			//IL_01f7: Expected Obj, but got Unknown
			//IL_02a3: Unknown result type (might be due to invalid IL or missing references)
			//IL_02ad: Expected Obj, but got Unknown
			//IL_0309: Unknown result type (might be due to invalid IL or missing references)
			//IL_0313: Expected Obj, but got Unknown
			ComponentResourceManager componentResourceManager = new ComponentResourceManager(typeof(Form2));
			SaveFileDialog1 = new SaveFileDialog();
			Button1 = new Button();
			Button2 = new Button();
			Button3 = new Button();
			AxAcroPDF1 = new AxAcroPDF();
			Button4 = new Button();
			((ISupportInitialize)AxAcroPDF1).BeginInit();
			((Control)this).SuspendLayout();
			((ButtonBase)Button1).BackColor = Color.FromArgb(255, 255, 192);
			((Control)Button1).Font = new Font("Arial", 14.25f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Button1).Location = new Point(503, 12);
			((Control)Button1).Name = "Button1";
			((Control)Button1).Size = new Size(108, 36);
			((Control)Button1).TabIndex = 4;
			((ButtonBase)Button1).Text = "Print";
			((ButtonBase)Button1).UseVisualStyleBackColor = false;
			((ButtonBase)Button2).BackColor = Color.FromArgb(255, 255, 192);
			((Control)Button2).Font = new Font("Arial", 14.25f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Button2).Location = new Point(618, 12);
			((Control)Button2).Name = "Button2";
			((Control)Button2).Size = new Size(119, 36);
			((Control)Button2).TabIndex = 5;
			((ButtonBase)Button2).Text = "Download";
			((ButtonBase)Button2).UseVisualStyleBackColor = false;
			((ButtonBase)Button3).BackColor = Color.FromArgb(255, 255, 192);
			((Control)Button3).Font = new Font("Arial", 14.25f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Button3).Location = new Point(744, 12);
			((Control)Button3).Name = "Button3";
			((Control)Button3).Size = new Size(108, 36);
			((Control)Button3).TabIndex = 6;
			((ButtonBase)Button3).Text = "Exit";
			((ButtonBase)Button3).UseVisualStyleBackColor = false;
			((AxHost)AxAcroPDF1).Enabled = true;
			((Control)AxAcroPDF1).Location = new Point(0, -1);
			((Control)AxAcroPDF1).Name = "AxAcroPDF1";
			((AxHost)AxAcroPDF1).OcxState = (State)componentResourceManager.GetObject("AxAcroPDF1.OcxState");
			((Control)AxAcroPDF1).Size = new Size(1532, 997);
			((Control)AxAcroPDF1).TabIndex = 7;
			((ButtonBase)Button4).BackColor = Color.FromArgb(255, 255, 192);
			((Control)Button4).Font = new Font("Arial", 14.25f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Button4).Location = new Point(389, 12);
			((Control)Button4).Name = "Button4";
			((Control)Button4).Size = new Size(108, 36);
			((Control)Button4).TabIndex = 8;
			((ButtonBase)Button4).Text = "Preview";
			((ButtonBase)Button4).UseVisualStyleBackColor = false;
			((ContainerControl)this).AutoScaleDimensions = new SizeF(6f, 13f);
			((ContainerControl)this).AutoScaleMode = (AutoScaleMode)1;
			((Form)this).ClientSize = new Size(1533, 825);
			((Control)this).Controls.Add((Control)(object)Button4);
			((Control)this).Controls.Add((Control)(object)Button3);
			((Control)this).Controls.Add((Control)(object)Button2);
			((Control)this).Controls.Add((Control)(object)Button1);
			((Control)this).Controls.Add((Control)(object)AxAcroPDF1);
			((Control)this).Name = "Form2";
			((Form)this).Text = "Utility For Make E-Invoice / E-Way Bill Automatically From Bahi-Khata Software";
			((Form)this).WindowState = (FormWindowState)2;
			((ISupportInitialize)AxAcroPDF1).EndInit();
			((Control)this).ResumeLayout(false);
		}

		private void Form2_Load(object sender, EventArgs e)
		{
			Application.DoEvents();
			load_pdf_path = Module1.PDF_File_Path;
			Thread.Sleep(2000);
			AxAcroPDF1.src = load_pdf_path;
			((Control)AxAcroPDF1).Refresh();
		}

		private void Button1_Click(object sender, EventArgs e)
		{
			AxAcroPDF1.Print();
		}

		private void Button2_Click(object sender, EventArgs e)
		{
			//IL_0018: Unknown result type (might be due to invalid IL or missing references)
			//IL_001e: Invalid comparison between Unknown and I4
			((FileDialog)SaveFileDialog1).Filter = "PDF Files (*.pdf*)|*.pdf";
			string fileName = default;
			if ((int)((CommonDialog)SaveFileDialog1).ShowDialog() == 1)
			{
				fileName = ((FileDialog)SaveFileDialog1).FileName;
			}
			WebClient webClient = new WebClient();
			webClient.DownloadFile(load_pdf_path, fileName);
			Interaction.MsgBox("PDF Saved at : \r\n" + fileName, MsgBoxStyle.Information, "Information");
		}

		private void Button3_Click(object sender, EventArgs e)
		{
			((Component)this).Dispose();
			((Form)this).Close();
		}

		private void Button4_Click(object sender, EventArgs e)
		{
			((Control)AxAcroPDF1).Refresh();
		}
	}
	public class EWayWithoutIRN
	{
		public class PushData
		{
			public string EWBNumber { get; set; }

			public string CancelReasonCode { get; set; }

			public string CancelRemark { get; set; }

			public string CessNonAdvol { get; set; }

			public string CessNonAdvolValue { get; set; }

			public string CessRate { get; set; }

			public string CessValue { get; set; }

			public string dispatchFromGSTIN { get; set; }

			public string dispatchFromTradeName { get; set; }

			public string DocDate { get; set; }

			public string DocNo { get; set; }

			public string DocType { get; set; }

			public string EWBPassword { get; set; }

			public string EWBUserName { get; set; }

			public string GSTIN { get; set; }

			public string HSNCode { get; set; }

			public string IGSTRate { get; set; }

			public string IGSTValue { get; set; }

			public string Irn { get; set; }

			public string IsBillFromShipFromSame { get; set; }

			public string IsBillToShipToSame { get; set; }

			public string IsGSTINSEZ { get; set; }

			public string OtherValue { get; set; }

			public string ProductDesc { get; set; }

			public string ProductName { get; set; }

			public string QtyUnit { get; set; }

			public string Quantity { get; set; }

			public string RecAdd1 { get; set; }

			public string RecAdd2 { get; set; }

			public string Reccity { get; set; }

			public string RecGSTIN { get; set; }

			public string RecName { get; set; }

			public string Recpincode { get; set; }

			public string RecState { get; set; }

			public string ShipFromStateCode { get; set; }

			public string shipToGSTIN { get; set; }

			public string ShipToStateCode { get; set; }

			public string shipToTradeName { get; set; }

			public string SubSupplyDesc { get; set; }

			public string SubType { get; set; }

			public string SupAdd1 { get; set; }

			public string SupAdd2 { get; set; }

			public string SupCity { get; set; }

			public string SupGSTIN { get; set; }

			public string SupName { get; set; }

			public string SupPincode { get; set; }

			public string SupplyType { get; set; }

			public string SupState { get; set; }

			public string TaxableValue { get; set; }

			public string TotalInvoiceValue { get; set; }

			public string TransDistance { get; set; }

			public string TransDocDate { get; set; }

			public string Transdocno { get; set; }

			public string TransMode { get; set; }

			public string TransporterId { get; set; }

			public string TransporterName { get; set; }

			public string VehicleNo { get; set; }

			public string VehicleType { get; set; }
		}

		public class CancelEWayRequestModel
		{
			public List<PushData> Push_Data_List { get; set; }

			public string Year { get; set; }

			public string Month { get; set; }

			public string EFUserName { get; set; }

			public string EFPassword { get; set; }

			public string CDKey { get; set; }
		}
	}
	[StandardModule]
	internal sealed class Module1
	{
		public static bool Local = false;

		public static string Main_TABLE;

		public static string db_path;

		public static bool with_args;

		public static string PDF_File_Path;

		[STAThread]
		public static void Main(string[] args)
		{
			//IL_0040: Unknown result type (might be due to invalid IL or missing references)
			//IL_0167: Unknown result type (might be due to invalid IL or missing references)
			if (((ServerComputer)MyProject.Computer).FileSystem.FileExists(Application.StartupPath + "\\Local.txt"))
			{
				Local = true;
			}
			if (Local)
			{
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
				Main_TABLE = args[1];
				db_path = args[2];
				Main_TABLE = Main_TABLE.Replace("^", " ");
				db_path = db_path.Replace("^", " ");
				((Form)MyProject.Forms.Form1).ShowDialog();
			}
		}
	}
}
