using System;
using System.CodeDom.Compiler;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Configuration;
using System.Data;
using System.Data.Common;
using System.Data.OleDb;
using System.Data.SqlClient;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.IO.Packaging;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Resources;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Web.Script.Serialization;
using System.Windows.Forms;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.ApplicationServices;
using Microsoft.VisualBasic.CompilerServices;
using Microsoft.VisualBasic.Devices;
using Microsoft.Win32;
using Newtonsoft.Json.Linq;
using SyncData.My;

[assembly: CompilationRelaxations(8)]
[assembly: RuntimeCompatibility(WrapNonExceptionThrows = true)]
[assembly: Debuggable(DebuggableAttribute.DebuggingModes.Default | DebuggableAttribute.DebuggingModes.DisableOptimizations | DebuggableAttribute.DebuggingModes.IgnoreSymbolStoreSequencePoints | DebuggableAttribute.DebuggingModes.EnableEditAndContinue)]
[assembly: AssemblyTitle("Kamra Softwares")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyCompany("")]
[assembly: AssemblyProduct("Kamra Softwares")]
[assembly: AssemblyCopyright("Copyright ©  2019")]
[assembly: AssemblyTrademark("")]
[assembly: ComVisible(false)]
[assembly: Guid("e744be11-137e-45df-96da-61b618bb1639")]
[assembly: AssemblyFileVersion("1.0.0.0")]
[assembly: TargetFramework(".NETFramework,Version=v4.7.2", FrameworkDisplayName = ".NET Framework 4.7.2")]
[assembly: AssemblyVersion("1.0.0.0")]
namespace SyncData.My
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
			public Change_details m_Change_details;

			[EditorBrowsable(EditorBrowsableState.Never)]
			public Form1 m_Form1;

			[EditorBrowsable(EditorBrowsableState.Never)]
			public Reg_User m_Reg_User;

			public Change_details Change_details
			{
				[DebuggerHidden]
				get
				{
					m_Change_details = Create__Instance__(m_Change_details);
					return m_Change_details;
				}
				[DebuggerHidden]
				set
				{
					if (value != m_Change_details)
					{
						if (value != null)
						{
							throw new ArgumentException("Property can only be set to Nothing");
						}
						Dispose__Instance__(ref m_Change_details);
					}
				}
			}

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

			public Reg_User Reg_User
			{
				[DebuggerHidden]
				get
				{
					m_Reg_User = Create__Instance__(m_Reg_User);
					return m_Reg_User;
				}
				[DebuggerHidden]
				set
				{
					if (value != m_Reg_User)
					{
						if (value != null)
						{
							throw new ArgumentException("Property can only be set to Nothing");
						}
						Dispose__Instance__(ref m_Reg_User);
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
namespace SyncData.My.Resources
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
					ResourceManager resourceManager = new ResourceManager("SyncData.Resources", typeof(Resources).Assembly);
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

		internal static Bitmap wait
		{
			get
			{
				//IL_001c: Unknown result type (might be due to invalid IL or missing references)
				//IL_0022: Expected Obj, but got Unknown
				object objectValue = RuntimeHelpers.GetObjectValue(ResourceManager.GetObject("wait", resourceCulture));
				return (Bitmap)objectValue;
			}
		}

		internal static Bitmap wait1
		{
			get
			{
				//IL_001c: Unknown result type (might be due to invalid IL or missing references)
				//IL_0022: Expected Obj, but got Unknown
				object objectValue = RuntimeHelpers.GetObjectValue(ResourceManager.GetObject("wait1", resourceCulture));
				return (Bitmap)objectValue;
			}
		}
	}
}
namespace SyncData.My
{
	[CompilerGenerated]
	[GeneratedCode("Microsoft.VisualStudio.Editors.SettingsDesigner.SettingsSingleFileGenerator", "16.3.0.0")]
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
namespace SyncData
{
	[DesignerGenerated]
	public class Change_details : Form
	{
		private IContainer components;

		private OleDbConnection con;

		private string suc;

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

		[field: AccessedThroughProperty("GroupBox2")]
		internal virtual GroupBox GroupBox2
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

		[field: AccessedThroughProperty("Label1")]
		internal virtual Label Label1
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("txt_UserEmail")]
		internal virtual TextBox txt_UserEmail
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

		[field: AccessedThroughProperty("txtUserMobile")]
		internal virtual TextBox txtUserMobile
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("GroupBox1")]
		internal virtual GroupBox GroupBox1
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

		[field: AccessedThroughProperty("txtPassword")]
		internal virtual TextBox txtPassword
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("Label3")]
		internal virtual Label Label3
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("GroupBox3")]
		internal virtual GroupBox GroupBox3
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
				EventHandler eventHandler = Button1_Click_1;
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

		[field: AccessedThroughProperty("Label4")]
		internal virtual Label Label4
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("txtNewEmail")]
		internal virtual TextBox txtNewEmail
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("Label5")]
		internal virtual Label Label5
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("txtNewMobile")]
		internal virtual TextBox txtNewMobile
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		public Change_details()
		{
			//IL_001b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0025: Expected Obj, but got Unknown
			((Form)this).Load += Change_details_Load;
			con = new OleDbConnection();
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
			//IL_0086: Unknown result type (might be due to invalid IL or missing references)
			//IL_0090: Expected Obj, but got Unknown
			//IL_0092: Unknown result type (might be due to invalid IL or missing references)
			//IL_009c: Expected Obj, but got Unknown
			//IL_009e: Unknown result type (might be due to invalid IL or missing references)
			//IL_00a8: Expected Obj, but got Unknown
			//IL_00aa: Unknown result type (might be due to invalid IL or missing references)
			//IL_00b4: Expected Obj, but got Unknown
			//IL_00b6: Unknown result type (might be due to invalid IL or missing references)
			//IL_00c0: Expected Obj, but got Unknown
			//IL_00c2: Unknown result type (might be due to invalid IL or missing references)
			//IL_00cc: Expected Obj, but got Unknown
			//IL_00ce: Unknown result type (might be due to invalid IL or missing references)
			//IL_00d8: Expected Obj, but got Unknown
			//IL_00da: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e4: Expected Obj, but got Unknown
			//IL_00e6: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f0: Expected Obj, but got Unknown
			//IL_013a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0144: Expected Obj, but got Unknown
			//IL_02b5: Unknown result type (might be due to invalid IL or missing references)
			//IL_02bf: Expected Obj, but got Unknown
			//IL_0347: Unknown result type (might be due to invalid IL or missing references)
			//IL_0351: Expected Obj, but got Unknown
			//IL_03db: Unknown result type (might be due to invalid IL or missing references)
			//IL_03e5: Expected Obj, but got Unknown
			//IL_0477: Unknown result type (might be due to invalid IL or missing references)
			//IL_0481: Expected Obj, but got Unknown
			//IL_04fb: Unknown result type (might be due to invalid IL or missing references)
			//IL_0505: Expected Obj, but got Unknown
			//IL_0598: Unknown result type (might be due to invalid IL or missing references)
			//IL_05a2: Expected Obj, but got Unknown
			//IL_06f1: Unknown result type (might be due to invalid IL or missing references)
			//IL_06fb: Expected Obj, but got Unknown
			//IL_0784: Unknown result type (might be due to invalid IL or missing references)
			//IL_078e: Expected Obj, but got Unknown
			//IL_0809: Unknown result type (might be due to invalid IL or missing references)
			//IL_0813: Expected Obj, but got Unknown
			//IL_089e: Unknown result type (might be due to invalid IL or missing references)
			//IL_08a8: Expected Obj, but got Unknown
			//IL_0a31: Unknown result type (might be due to invalid IL or missing references)
			//IL_0a3b: Expected Obj, but got Unknown
			//IL_0ac3: Unknown result type (might be due to invalid IL or missing references)
			//IL_0acd: Expected Obj, but got Unknown
			//IL_0b57: Unknown result type (might be due to invalid IL or missing references)
			//IL_0b61: Expected Obj, but got Unknown
			//IL_0be7: Unknown result type (might be due to invalid IL or missing references)
			//IL_0bf1: Expected Obj, but got Unknown
			//IL_0c5e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0c68: Expected Obj, but got Unknown
			//IL_0cee: Unknown result type (might be due to invalid IL or missing references)
			//IL_0cf8: Expected Obj, but got Unknown
			Button2 = new Button();
			GroupBox2 = new GroupBox();
			Button3 = new Button();
			Button4 = new Button();
			Label1 = new Label();
			txt_UserEmail = new TextBox();
			Label2 = new Label();
			txtUserMobile = new TextBox();
			GroupBox1 = new GroupBox();
			Button5 = new Button();
			Button6 = new Button();
			txtPassword = new TextBox();
			Label3 = new Label();
			GroupBox3 = new GroupBox();
			Button1 = new Button();
			Button7 = new Button();
			Label4 = new Label();
			txtNewEmail = new TextBox();
			Label5 = new Label();
			txtNewMobile = new TextBox();
			((Control)GroupBox2).SuspendLayout();
			((Control)GroupBox1).SuspendLayout();
			((Control)GroupBox3).SuspendLayout();
			((Control)this).SuspendLayout();
			((ButtonBase)Button2).FlatStyle = (FlatStyle)0;
			((Control)Button2).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Button2).Location = new Point(541, 392);
			((Control)Button2).Name = "Button2";
			((Control)Button2).Size = new Size(92, 31);
			((Control)Button2).TabIndex = 8;
			((ButtonBase)Button2).Text = "Exit";
			((ButtonBase)Button2).UseVisualStyleBackColor = true;
			((Control)GroupBox2).Controls.Add((Control)(object)Button3);
			((Control)GroupBox2).Controls.Add((Control)(object)Button4);
			((Control)GroupBox2).Controls.Add((Control)(object)Label1);
			((Control)GroupBox2).Controls.Add((Control)(object)txt_UserEmail);
			((Control)GroupBox2).Controls.Add((Control)(object)Label2);
			((Control)GroupBox2).Controls.Add((Control)(object)txtUserMobile);
			((Control)GroupBox2).Location = new Point(12, 12);
			((Control)GroupBox2).Name = "GroupBox2";
			((Control)GroupBox2).Size = new Size(512, 139);
			((Control)GroupBox2).TabIndex = 9;
			GroupBox2.TabStop = false;
			((ButtonBase)Button3).FlatStyle = (FlatStyle)0;
			((Control)Button3).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Button3).Location = new Point(130, 95);
			((Control)Button3).Name = "Button3";
			((Control)Button3).Size = new Size(108, 31);
			((Control)Button3).TabIndex = 2;
			((ButtonBase)Button3).Text = "Change Info";
			((ButtonBase)Button3).UseVisualStyleBackColor = true;
			((ButtonBase)Button4).FlatStyle = (FlatStyle)0;
			((Control)Button4).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Button4).Location = new Point(245, 95);
			((Control)Button4).Name = "Button4";
			((Control)Button4).Size = new Size(92, 31);
			((Control)Button4).TabIndex = 3;
			((ButtonBase)Button4).Text = "Exit";
			((ButtonBase)Button4).UseVisualStyleBackColor = true;
			Label1.AutoSize = true;
			((Control)Label1).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Label1).ForeColor = Color.FromArgb(192, 0, 0);
			((Control)Label1).Location = new Point(4, 21);
			((Control)Label1).Name = "Label1";
			((Control)Label1).Size = new Size(195, 17);
			((Control)Label1).TabIndex = 1;
			Label1.Text = "Your Existing Mobile No. :";
			((Control)txt_UserEmail).Enabled = false;
			((Control)txt_UserEmail).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)txt_UserEmail).Location = new Point(177, 48);
			((Control)txt_UserEmail).Name = "txt_UserEmail";
			((TextBoxBase)txt_UserEmail).ReadOnly = true;
			((Control)txt_UserEmail).Size = new Size(327, 23);
			((Control)txt_UserEmail).TabIndex = 1;
			Label2.AutoSize = true;
			((Control)Label2).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Label2).ForeColor = Color.FromArgb(192, 0, 0);
			((Control)Label2).Location = new Point(15, 50);
			((Control)Label2).Name = "Label2";
			((Control)Label2).Size = new Size(183, 17);
			((Control)Label2).TabIndex = 3;
			Label2.Text = "Your Existing E-mail ID :";
			((Control)txtUserMobile).Enabled = false;
			((Control)txtUserMobile).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)txtUserMobile).Location = new Point(177, 19);
			((TextBoxBase)txtUserMobile).MaxLength = 10;
			((Control)txtUserMobile).Name = "txtUserMobile";
			((TextBoxBase)txtUserMobile).ReadOnly = true;
			((Control)txtUserMobile).Size = new Size(199, 23);
			((Control)txtUserMobile).TabIndex = 0;
			((Control)GroupBox1).Controls.Add((Control)(object)Button5);
			((Control)GroupBox1).Controls.Add((Control)(object)Button6);
			((Control)GroupBox1).Controls.Add((Control)(object)txtPassword);
			((Control)GroupBox1).Controls.Add((Control)(object)Label3);
			((Control)GroupBox1).Location = new Point(12, 143);
			((Control)GroupBox1).Name = "GroupBox1";
			((Control)GroupBox1).Size = new Size(512, 132);
			((Control)GroupBox1).TabIndex = 7;
			GroupBox1.TabStop = false;
			((Control)GroupBox1).Visible = false;
			((ButtonBase)Button5).FlatStyle = (FlatStyle)0;
			((Control)Button5).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Button5).Location = new Point(270, 77);
			((Control)Button5).Name = "Button5";
			((Control)Button5).Size = new Size(81, 27);
			((Control)Button5).TabIndex = 9;
			((ButtonBase)Button5).Text = "Exit";
			((ButtonBase)Button5).UseVisualStyleBackColor = true;
			((ButtonBase)Button6).FlatStyle = (FlatStyle)0;
			((Control)Button6).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Button6).Location = new Point(155, 77);
			((Control)Button6).Name = "Button6";
			((Control)Button6).Size = new Size(92, 27);
			((Control)Button6).TabIndex = 8;
			((ButtonBase)Button6).Text = "OK";
			((ButtonBase)Button6).UseVisualStyleBackColor = true;
			((Control)txtPassword).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)txtPassword).Location = new Point(223, 38);
			((TextBoxBase)txtPassword).MaxLength = 10;
			((Control)txtPassword).Name = "txtPassword";
			txtPassword.PasswordChar = '*';
			((Control)txtPassword).Size = new Size(199, 23);
			((Control)txtPassword).TabIndex = 6;
			Label3.AutoSize = true;
			((Control)Label3).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Label3).ForeColor = Color.FromArgb(192, 0, 0);
			((Control)Label3).Location = new Point(18, 40);
			((Control)Label3).Name = "Label3";
			((Control)Label3).Size = new Size(203, 17);
			((Control)Label3).TabIndex = 7;
			Label3.Text = "Enter Your App Password :";
			((Control)GroupBox3).Controls.Add((Control)(object)Button1);
			((Control)GroupBox3).Controls.Add((Control)(object)Button7);
			((Control)GroupBox3).Controls.Add((Control)(object)Label4);
			((Control)GroupBox3).Controls.Add((Control)(object)txtNewEmail);
			((Control)GroupBox3).Controls.Add((Control)(object)Label5);
			((Control)GroupBox3).Controls.Add((Control)(object)txtNewMobile);
			((Control)GroupBox3).Location = new Point(12, 275);
			((Control)GroupBox3).Name = "GroupBox3";
			((Control)GroupBox3).Size = new Size(512, 148);
			((Control)GroupBox3).TabIndex = 10;
			GroupBox3.TabStop = false;
			((Control)GroupBox3).Visible = false;
			((ButtonBase)Button1).FlatStyle = (FlatStyle)0;
			((Control)Button1).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Button1).Location = new Point(130, 95);
			((Control)Button1).Name = "Button1";
			((Control)Button1).Size = new Size(102, 31);
			((Control)Button1).TabIndex = 2;
			((ButtonBase)Button1).Text = "Save && Exit";
			((ButtonBase)Button1).UseVisualStyleBackColor = true;
			((ButtonBase)Button7).FlatStyle = (FlatStyle)0;
			((Control)Button7).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Button7).Location = new Point(245, 95);
			((Control)Button7).Name = "Button7";
			((Control)Button7).Size = new Size(92, 31);
			((Control)Button7).TabIndex = 3;
			((ButtonBase)Button7).Text = "Exit";
			((ButtonBase)Button7).UseVisualStyleBackColor = true;
			Label4.AutoSize = true;
			((Control)Label4).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Label4).ForeColor = Color.FromArgb(192, 0, 0);
			((Control)Label4).Location = new Point(13, 21);
			((Control)Label4).Name = "Label4";
			((Control)Label4).Size = new Size(174, 17);
			((Control)Label4).TabIndex = 1;
			Label4.Text = "Enter New Mobile No. :";
			((Control)txtNewEmail).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)txtNewEmail).Location = new Point(171, 48);
			((Control)txtNewEmail).Name = "txtNewEmail";
			((Control)txtNewEmail).Size = new Size(314, 23);
			((Control)txtNewEmail).TabIndex = 1;
			Label5.AutoSize = true;
			((Control)Label5).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Label5).ForeColor = Color.FromArgb(192, 0, 0);
			((Control)Label5).Location = new Point(22, 50);
			((Control)Label5).Name = "Label5";
			((Control)Label5).Size = new Size(162, 17);
			((Control)Label5).TabIndex = 3;
			Label5.Text = "Enter New E-mail ID :";
			((Control)txtNewMobile).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)txtNewMobile).Location = new Point(171, 19);
			((TextBoxBase)txtNewMobile).MaxLength = 10;
			((Control)txtNewMobile).Name = "txtNewMobile";
			((Control)txtNewMobile).Size = new Size(199, 23);
			((Control)txtNewMobile).TabIndex = 0;
			((ContainerControl)this).AutoScaleDimensions = new SizeF(6f, 13f);
			((ContainerControl)this).AutoScaleMode = (AutoScaleMode)1;
			((Form)this).ClientSize = new Size(659, 431);
			((Control)this).Controls.Add((Control)(object)GroupBox3);
			((Control)this).Controls.Add((Control)(object)GroupBox1);
			((Control)this).Controls.Add((Control)(object)GroupBox2);
			((Control)this).Controls.Add((Control)(object)Button2);
			((Control)this).Name = "Change_details";
			((Form)this).StartPosition = (FormStartPosition)1;
			((Form)this).Text = "Change User Information";
			((Control)GroupBox2).ResumeLayout(false);
			((Control)GroupBox2).PerformLayout();
			((Control)GroupBox1).ResumeLayout(false);
			((Control)GroupBox1).PerformLayout();
			((Control)GroupBox3).ResumeLayout(false);
			((Control)GroupBox3).PerformLayout();
			((Control)this).ResumeLayout(false);
		}

		private void Button2_Click(object sender, EventArgs e)
		{
			((Component)this).Dispose();
			((Form)this).Close();
		}

		public void conn()
		{
			if (con.State == ConnectionState.Open)
			{
				con.Close();
			}
			string startupPath = Application.StartupPath;
			string text = "";
			string text2 = Application.StartupPath + "\\control.lsp";
			con.ConnectionString = "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + text2 + ";Jet OLEDB:Database Password=jay_hanuman;";
			con.Open();
		}

		private void Change_details_Load(object sender, EventArgs e)
		{
			((Control)this).Height = 210;
			((Control)this).Width = 550;
			conn();
			txt_UserEmail.Text = MyProject.Forms.Reg_User.txt_UserEmail.Text;
			txtUserMobile.Text = MyProject.Forms.Reg_User.txtUserMobile.Text;
			txtNewEmail.Text = MyProject.Forms.Reg_User.txt_UserEmail.Text;
			txtNewMobile.Text = MyProject.Forms.Reg_User.txtUserMobile.Text;
		}

		private void Button1_Click(object sender, EventArgs e)
		{
			//IL_011d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0124: Expected Obj, but got Unknown
			//IL_0148: Unknown result type (might be due to invalid IL or missing references)
			if (Module1.CheckForInternetConnection())
			{
				string text = "pc";
				string text2 = txtUserMobile.Text;
				string text3 = To_MD5(txtPassword.Text);
				string text4 = default;
				string text5 = default;
				string text6 = default;
				string args = "{\"mobileno\":\"" + text4 + "\",\"email\":\"" + text5 + "\",\"password\":\"" + text3 + "\",\"new_mobileno\":\"" + text2 + "\",\"new_email\":\"" + text6 + "\",\"pc\":\"" + text + "\"}";
				string url = Module1.URL + "web-service/index.php?service=user_changeinfopc";
				upload_data_1(url, args);
				if (Operators.CompareString(Module1.userid, "", TextCompare: false) != 0)
				{
					string text7 = toBase64(txtUserMobile.Text);
					string text9 = default;
					string text8 = "UPDATE LabourSettings set LabourOnBag='" + text9 + "',LabourOnWeight='" + text7 + "'";
					OleDbCommand val = new OleDbCommand(text8, con);
					val.ExecuteNonQuery();
					Interaction.MsgBox("User Details Changed Successfully", MsgBoxStyle.Information, Module1.Title_msg);
					((Form)MyProject.Forms.Form1).ShowDialog();
				}
			}
			else
			{
				Interaction.MsgBox("Please Check your internet connection", MsgBoxStyle.Information, Module1.Title_msg);
			}
		}

		private string To_MD5(string s)
		{
			StringBuilder stringBuilder = new StringBuilder();
			byte[] bytes = Encoding.Default.GetBytes(s);
			bytes = MD5.Create().ComputeHash(bytes);
			checked
			{
				int num = bytes.Length - 1;
				for (int i = 0; i <= num; i++)
				{
					stringBuilder.Append(bytes[i].ToString("x2"));
				}
				return stringBuilder.ToString();
			}
		}

		public string toBase64(string s)
		{
			byte[] bytes = Encoding.ASCII.GetBytes(s);
			return Convert.ToBase64String(bytes).ToString();
		}

		private void upload_data_1(string url, string args)
		{
			Uri uri = new Uri(url);
			byte[] bytes = Encoding.UTF8.GetBytes(args);
			string text = SendRequest(uri, bytes, "application/json", "POST");
			JObject jObject = default;
			if (Operators.CompareString(text, "", TextCompare: false) != 0)
			{
				jObject = JObject.Parse(text);
			}
			if (Operators.CompareString(jObject["success"].ToString(), "1", TextCompare: false) == 0)
			{
				suc = "1";
			}
			else if (Operators.CompareString(jObject["success"].ToString(), "2", TextCompare: false) == 0)
			{
				((Control)GroupBox1).Visible = true;
				Point location = new Point(10, 10);
				((Control)GroupBox2).Visible = false;
				((Control)GroupBox1).Visible = true;
				((Control)GroupBox1).Location = location;
				((Control)txtPassword).Select();
				suc = "2";
			}
			else if (Operators.CompareString(jObject["success"].ToString(), "3", TextCompare: false) == 0)
			{
				((Control)GroupBox3).Visible = true;
				Point location2 = new Point(10, 10);
				((Control)GroupBox1).Visible = false;
				((Control)GroupBox3).Visible = true;
				((Control)GroupBox3).Location = location2;
				((Control)txtNewMobile).Select();
				suc = "3";
			}
			else
			{
				suc = "";
				Interaction.MsgBox(jObject["msg"].ToString().Replace("\"", ""), MsgBoxStyle.OkOnly, "Kamra Softwares");
			}
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

		private void Button3_Click(object sender, EventArgs e)
		{
			try
			{
				if (Module1.CheckForInternetConnection())
				{
					string text = "pc";
					string text2 = txt_UserEmail.Text;
					string text3 = txtUserMobile.Text;
					string args = "{\"mobileno\":\"" + text3 + "\",\"email\":\"" + text2 + "\",\"pc\":\"" + text + "\"}";
					string url = Module1.URL + "web-service/index.php?service=user_changeinfopc";
					upload_data_1(url, args);
					if (Operators.CompareString(Module1.userid, "", TextCompare: false) != 0)
					{
					}
				}
				else
				{
					Interaction.MsgBox("Please Check your internet connection", MsgBoxStyle.Information, Module1.Title_msg);
				}
			}
			catch (Exception ex)
			{
				ProjectData.SetProjectError(ex);
				Exception ex2 = ex;
				Interaction.MsgBox(ex2.ToString());
				ProjectData.ClearProjectError();
			}
		}

		private void Button6_Click(object sender, EventArgs e)
		{
			try
			{
				if (Module1.CheckForInternetConnection())
				{
					string text = "pc";
					string text2 = txt_UserEmail.Text;
					string text3 = txtUserMobile.Text;
					string text4 = To_MD5(txtPassword.Text);
					string args = "{\"mobileno\":\"" + text3 + "\",\"email\":\"" + text2 + "\",\"password\":\"" + text4 + "\",\"pc\":\"" + text + "\"}";
					string url = Module1.URL + "web-service/index.php?service=user_changeinfopc";
					upload_data_1(url, args);
				}
			}
			catch (Exception ex)
			{
				ProjectData.SetProjectError(ex);
				Exception ex2 = ex;
				Interaction.MsgBox(ex2.ToString());
				ProjectData.ClearProjectError();
			}
		}

		private void Button4_Click(object sender, EventArgs e)
		{
			((Component)this).Dispose();
			((Form)this).Close();
		}

		private void Button5_Click(object sender, EventArgs e)
		{
			((Component)this).Dispose();
			((Form)this).Close();
		}

		private void Button1_Click_1(object sender, EventArgs e)
		{
			//IL_0154: Unknown result type (might be due to invalid IL or missing references)
			//IL_015b: Expected Obj, but got Unknown
			try
			{
				if (Module1.CheckForInternetConnection())
				{
					string text = "pc";
					string text2 = txt_UserEmail.Text;
					string text3 = txtUserMobile.Text;
					string text4 = txtNewEmail.Text;
					string text5 = txtNewMobile.Text;
					string text6 = To_MD5(txtPassword.Text);
					string args = "{\"mobileno\":\"" + text3 + "\",\"email\":\"" + text2 + "\",\"password\":\"" + text6 + "\",\"new_mobileno\":\"" + text5 + "\",\"new_email\":\"" + text4 + "\",\"pc\":\"" + text + "\"}";
					string url = Module1.URL + "web-service/index.php?service=user_changeinfopc";
					upload_data_1(url, args);
					if (Operators.CompareString(suc, "1", TextCompare: false) == 0)
					{
						string text7 = toBase64(txtNewEmail.Text);
						string text8 = toBase64(txtNewMobile.Text);
						string text9 = "UPDATE LabourSettings set LabourOnBag='" + text7 + "',LabourOnWeight='" + text8 + "'";
						OleDbCommand val = new OleDbCommand(text9, con);
						val.ExecuteNonQuery();
						MyProject.Forms.Reg_User.txt_UserEmail.Text = txtNewEmail.Text;
						MyProject.Forms.Reg_User.txtUserMobile.Text = txtNewMobile.Text;
						Interaction.MsgBox("User Details Changed Successfully", MsgBoxStyle.Information, Module1.Title_msg);
						((Component)this).Dispose();
						((Form)this).Close();
						((Control)MyProject.Forms.Form1).Show();
					}
				}
			}
			catch (Exception ex)
			{
				ProjectData.SetProjectError(ex);
				Exception ex2 = ex;
				Interaction.MsgBox(ex2.ToString());
				ProjectData.ClearProjectError();
			}
		}

		private void Button7_Click(object sender, EventArgs e)
		{
			((Component)this).Dispose();
			((Form)this).Close();
		}
	}
	[DesignerGenerated]
	public class Form1 : Form
	{
		public enum ProgressBarTextLocation
		{
			Left,
			Centered
		}

		public class Paras
		{
			public Test[] test { get; set; }
		}

		public class Test
		{
			public string sub { get; set; }

			public string content { get; set; }
		}

		private class CSharpImpl
		{
			[Obsolete("Please refactor calling code to use normal Visual Basic assignment")]
			public static T __Assign<T>(ref T target, T value)
			{
				target = value;
				return value;
			}
		}

		public OleDbConnection con;

		private OleDbDataAdapter da;

		private DataSet ds;

		private DataTable dt;

		private OleDbCommand cmd;

		private string combined_json;

		private bool process_finish;

		private string company_id;

		private string company_name;

		private string firmshort_name;

		private string last_update_date;

		private string max_vch_date;

		private string year;

		private string file1;

		private string firm_state;

		private string cursor_file;

		private string ftp_username;

		private string ftp_password;

		private string ftp_remote_host;

		private int TOT_selected_cmps;

		private bool field_exists;

		private string GSTIN;

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

		[field: AccessedThroughProperty("DataGridView1")]
		internal virtual DataGridView DataGridView1
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

		[field: AccessedThroughProperty("pb1")]
		internal virtual ProgressBar pb1
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = pb1_Click;
				ProgressBar val = field;
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

		[field: AccessedThroughProperty("btn_add_fields")]
		internal virtual Button btn_add_fields
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = btn_add_fields_Click;
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

		[field: AccessedThroughProperty("btn_Fetch_data")]
		internal virtual Button btn_Fetch_data
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = btn_Fetch_data_Click;
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
				EventHandler eventHandler = Button6_Click_1;
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

		[field: AccessedThroughProperty("Label2")]
		internal virtual Label Label2
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("DataGridView2")]
		internal virtual DataGridView DataGridView2
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
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

		[field: AccessedThroughProperty("Label3")]
		internal virtual Label Label3
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("lblDatapath")]
		internal virtual Label lblDatapath
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("Label4")]
		internal virtual Label Label4
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("grid3")]
		internal virtual DataGridView grid3
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				//IL_0007: Unknown result type (might be due to invalid IL or missing references)
				//IL_000d: Expected Obj, but got Unknown
				//IL_0014: Unknown result type (might be due to invalid IL or missing references)
				//IL_001a: Expected Obj, but got Unknown
				//IL_002e: Unknown result type (might be due to invalid IL or missing references)
				//IL_0034: Expected Obj, but got Unknown
				//IL_003b: Unknown result type (might be due to invalid IL or missing references)
				//IL_0042: Expected Obj, but got Unknown
				//IL_0049: Unknown result type (might be due to invalid IL or missing references)
				//IL_0050: Expected Obj, but got Unknown
				//IL_0057: Unknown result type (might be due to invalid IL or missing references)
				//IL_005e: Expected Obj, but got Unknown
				//IL_0065: Unknown result type (might be due to invalid IL or missing references)
				//IL_006c: Expected Obj, but got Unknown
				//IL_0073: Unknown result type (might be due to invalid IL or missing references)
				//IL_007a: Expected Obj, but got Unknown
				//IL_008f: Unknown result type (might be due to invalid IL or missing references)
				//IL_0096: Expected Obj, but got Unknown
				DataGridViewCellEventHandler val = grid3_CellContentClick;
				KeyEventHandler val2 = grid3_KeyDown;
				EventHandler eventHandler = grid3_Click;
				DataGridViewCellStateChangedEventHandler val3 = grid3_CellStateChanged;
				DataGridViewCellEventHandler val4 = grid3_CellValueChanged;
				DataGridViewCellMouseEventHandler val5 = grid3_CellMouseClick;
				DataGridViewCellEventHandler val6 = grid3_CellEndEdit;
				MouseEventHandler val7 = grid3_MouseClick;
				MouseEventHandler val8 = grid3_MouseUp;
				EventHandler eventHandler2 = grid3_Leave;
				DataGridViewCellEventHandler val9 = grid3_CellDoubleClick;
				DataGridView val10 = field;
				if (val10 != null)
				{
					val10.CellContentClick -= val;
					((Control)val10).KeyDown -= val2;
					((Control)val10).Click -= eventHandler;
					val10.CellStateChanged -= val3;
					val10.CellValueChanged -= val4;
					val10.CellMouseClick -= val5;
					val10.CellEndEdit -= val6;
					((Control)val10).MouseClick -= val7;
					((Control)val10).MouseUp -= val8;
					((Control)val10).Leave -= eventHandler2;
					val10.CellDoubleClick -= val9;
				}
				field = value;
				val10 = field;
				if (val10 != null)
				{
					val10.CellContentClick += val;
					((Control)val10).KeyDown += val2;
					((Control)val10).Click += eventHandler;
					val10.CellStateChanged += val3;
					val10.CellValueChanged += val4;
					val10.CellMouseClick += val5;
					val10.CellEndEdit += val6;
					((Control)val10).MouseClick += val7;
					((Control)val10).MouseUp += val8;
					((Control)val10).Leave += eventHandler2;
					val10.CellDoubleClick += val9;
				}
			}
		}

		[field: AccessedThroughProperty("Button8")]
		internal virtual Button Button8
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = Button8_Click;
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

		[field: AccessedThroughProperty("Lbl_UserID")]
		internal virtual Label Lbl_UserID
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("lbl_sel_cmps")]
		internal virtual Label lbl_sel_cmps
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("CHKRewrite")]
		internal virtual CheckBox CHKRewrite
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("Timer1")]
		internal virtual Timer Timer1
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = Timer1_Tick;
				Timer val = field;
				if (val != null)
				{
					val.Tick -= eventHandler;
				}
				field = value;
				val = field;
				if (val != null)
				{
					val.Tick += eventHandler;
				}
			}
		}

		public Form1()
		{
			//IL_0022: Unknown result type (might be due to invalid IL or missing references)
			//IL_002c: Expected Obj, but got Unknown
			//IL_0041: Unknown result type (might be due to invalid IL or missing references)
			//IL_004b: Expected Obj, but got Unknown
			//IL_004c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0056: Expected Obj, but got Unknown
			((Form)this).Load += Form1_Load;
			((Control)this).KeyDown += Form1_KeyDown;
			((Form)this).Closing += Form1_Closing;
			con = new OleDbConnection();
			cmd = new OleDbCommand();
			InitializeComponent();
		}

		private void Button1_Click(object sender, EventArgs e)
		{
			//IL_001b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0025: Expected Obj, but got Unknown
			string text = "Select RowNo,VoucherNumber,Format([VoucherDate],'dd-mm-yyyy') as VoucherDate,TransType,AccountCode,DrCr,Amount,Narration,BalAmount,InvoiceNo from Transactions";
			text = "SELECT Transactions.RowNo, Transactions.VoucherNumber, Format([Transactions.VoucherDate],'dd-mm-yyyy') as VoucherDate, Transactions.TransType, Transactions.AccountCode, Transactions.DrCr, Transactions.Amount, Transactions.Narration, Transactions.BalAmount, Transactions.InvoiceNo, NewTable.details FROM Transactions INNER JOIN NewTable ON Transactions.VoucherNumber = NewTable.VoucherNo";
			text = "SELECT Transactions.RowNo, Transactions.VoucherNumber, Format([Transactions.VoucherDate], 'dd-mm-yyyy') AS VoucherDate, Transactions.TransType, Transactions.AccountCode, Transactions.DrCr,  Transactions.Amount, Transactions.Narration, Transactions.BalAmount, Transactions.InvoiceNo, NewTable.details FROM Transactions LEFT OUTER JOIN NewTable ON Transactions.TransType = NewTable.VoucherType AND Transactions.VoucherNumber = NewTable.VoucherNo";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			string json = GetJson(dt);
			string text2 = Module1.db_path + "\\" + file1 + ".txt";
			FileStream fileStream = File.Create(text2);
			byte[] bytes = Encoding.UTF8.GetBytes(json);
			string s = Convert.ToBase64String(bytes);
			bytes = Encoding.UTF8.GetBytes(s);
			fileStream.Write(bytes, 0, bytes.Length);
			fileStream.Close();
			FileStream fileStream2 = File.Create(text2);
			byte[] bytes2 = new UTF8Encoding(encoderShouldEmitUTF8Identifier: true).GetBytes(json);
			fileStream2.Write(bytes2, 0, bytes2.Length);
			fileStream2.Close();
			string source = text2;
			string destination = Module1.db_path + "\\" + file1 + ".zip";
			CreateZipFile(source, destination);
			Interaction.MsgBox("Process Completed");
		}

		private void FTP_upload_file(string fnm)
		{
			string args = "{\"images\":\"" + fnm + "\"}";
			Interaction.MsgBox(fnm);
			string url = "http://kamrasoftwares.com/bahikhata/fileupload.php";
			upload_data_FTP(url, args);
		}

		private void upload_data_FTP(string url, string args)
		{
			Uri uri = new Uri(url);
			byte[] bytes = Encoding.UTF8.GetBytes(args);
			string text = SendRequest(uri, bytes, "application/json", "POST");
		}

		private void FtpUploadFile(string filetoupload, string ftpuri, string ftpusername, string ftppassword)
		{
			//IL_0068: Unknown result type (might be due to invalid IL or missing references)
			FtpWebRequest ftpWebRequest = (FtpWebRequest)WebRequest.Create(ftpuri);
			ftpWebRequest.Method = "STOR";
			ftpWebRequest.Credentials = new NetworkCredential(ftpusername, ftppassword);
			byte[] array = File.ReadAllBytes(filetoupload);
			ftpWebRequest.ContentLength = array.Length;
			using (Stream stream = ftpWebRequest.GetRequestStream())
			{
				stream.Write(array, 0, array.Length);
				stream.Close();
			}
			MessageBox.Show("Process Complete");
		}

		public byte[] CompressByte(byte[] byteSource)
		{
			MemoryStream memoryStream = new MemoryStream();
			GZipStream gZipStream = new GZipStream(memoryStream, CompressionMode.Compress, leaveOpen: true);
			gZipStream.Write(byteSource, 0, byteSource.Length);
			gZipStream.Dispose();
			memoryStream.Position = 0L;
			byte[] array = new byte[checked((int)memoryStream.Length + 1)];
			memoryStream.Read(array, 0, array.Length);
			memoryStream.Dispose();
			return array;
		}

		public void conn(string dbnm)
		{
			if (con.State == ConnectionState.Open)
			{
				con.Close();
			}
			con.ConnectionString = "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + dbnm + ";Jet OLEDB:Database Password=jay_hanuman;";
			con.Open();
		}

		private string GetJson(DataTable dt)
		{
			//IL_0001: Unknown result type (might be due to invalid IL or missing references)
			//IL_0006: Unknown result type (might be due to invalid IL or missing references)
			//IL_0013: Expected Obj, but got Unknown
			JavaScriptSerializer val = new JavaScriptSerializer
			{
				MaxJsonLength = 86753090
			};
			List<Dictionary<string, object>> list = new List<Dictionary<string, object>>();
			foreach (DataRow row in dt.Rows)
			{
				Dictionary<string, object> dictionary = new Dictionary<string, object>();
				foreach (DataColumn column in dt.Columns)
				{
					dictionary.Add(column.ColumnName, RuntimeHelpers.GetObjectValue(row[column]));
				}
				list.Add(dictionary);
			}
			return val.Serialize((object)list);
		}

		private void Form1_Load(object sender, EventArgs e)
		{
			//IL_00f4: Unknown result type (might be due to invalid IL or missing references)
			//IL_00fa: Expected Obj, but got Unknown
			//IL_0125: Unknown result type (might be due to invalid IL or missing references)
			//IL_012f: Expected Obj, but got Unknown
			//IL_01b9: Unknown result type (might be due to invalid IL or missing references)
			//IL_01c3: Expected Obj, but got Unknown
			Module1.Title_msg = "Kamra Softwares";
			((Form)this).Text = "Synchronize Bahi-Khata Data to Your Android Device";
			Lbl_UserID.Text = "";
			Label1.Text = "";
			Label2.Text = "";
			Label4.Text = "";
			lbl_sel_cmps.Text = "";
			cursor_file = Application.StartupPath + "\\wait.ico";
			Thread.CurrentThread.CurrentCulture = new CultureInfo("en-IN", useUserOverride: true);
			Registry.SetValue("HKEY_CURRENT_USER\\Control Panel\\International", "sShortDate", "dd-MMM-yyyy");
			if (Operators.CompareString(Module1.db_path, "", TextCompare: false) == 0)
			{
				Module1.db_path = Application.StartupPath + "\\Data";
			}
			lblDatapath.Text = "Data Path :  " + Module1.db_path;
			OleDbConnection val = new OleDbConnection();
			val.ConnectionString = "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + Application.StartupPath + "\\control.lsp;";
			val.Open();
			string text = "SELECT UserID from LabourSettings";
			da = new OleDbDataAdapter(text, val);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			if (ds.Tables[0].Rows.Count > 0)
			{
				Lbl_UserID.Text = ds.Tables[0].Rows[0][0].ToString();
			}
			DataGridView val2 = grid3;
			val2.Font = new Font("Microsoft Sans Serif", 11f, (FontStyle)1);
			val2.ForeColor = Color.Black;
			val2.EnableHeadersVisualStyles = false;
			val2.ColumnHeadersHeight = 30;
			val2.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)1;
			val2.ColumnHeadersDefaultCellStyle.Alignment = (DataGridViewContentAlignment)32;
			val2.AutoSizeColumnsMode = (DataGridViewAutoSizeColumnsMode)2;
			val2.SelectionMode = (DataGridViewSelectionMode)1;
			val2.ColumnHeadersDefaultCellStyle.SelectionBackColor = ColorTranslator.FromHtml("#f0f0f0");
			val2 = null;
			grid3.RowCount = 1;
			grid3.ColumnCount = 6;
			grid3.Columns[0].HeaderText = "Firm Name  (Ctrl+A: Select All  -  Ctrl+U: Unselect All)             ";
			grid3.Columns[1].HeaderText = "Work Period ";
			grid3.Columns[2].HeaderText = "File             ";
			grid3.Columns[3].HeaderText = "Max Vch. Dt";
			grid3.Columns[5].HeaderText = "      ";
			grid3.DefaultCellStyle.BackColor = ColorTranslator.FromHtml("#ffffc0");
			grid3.Columns[2].Visible = false;
			GET_companies();
			grid3.Columns[0].ReadOnly = true;
			grid3.Columns[1].ReadOnly = true;
			grid3.Columns[2].ReadOnly = true;
			grid3.Columns[3].ReadOnly = true;
			grid3.Columns[0].SortMode = (DataGridViewColumnSortMode)0;
			grid3.Columns[1].SortMode = (DataGridViewColumnSortMode)0;
			grid3.Columns[2].SortMode = (DataGridViewColumnSortMode)0;
			grid3.Columns[3].SortMode = (DataGridViewColumnSortMode)0;
			grid3.Columns[4].SortMode = (DataGridViewColumnSortMode)0;
			grid3.Columns[2].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)32;
			grid3.Columns[4].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)32;
			grid3.Columns[6].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)32;
			Comp_ID();
			Count_selected_cmps();
			grid3.Sort(grid3.Columns[1], ListSortDirection.Ascending);
			grid3.Columns[6].Visible = false;
			((Control)Lbl_UserID).Visible = false;
		}

		private void ResetCompannyID(string db_file)
		{
			//IL_0103: Unknown result type (might be due to invalid IL or missing references)
			//IL_010d: Expected Obj, but got Unknown
			//IL_0069: Unknown result type (might be due to invalid IL or missing references)
			//IL_0073: Expected Obj, but got Unknown
			//IL_00b3: Unknown result type (might be due to invalid IL or missing references)
			//IL_00bd: Expected Obj, but got Unknown
			conn(db_file);
			string text = "OtherSettings";
			string text2 = "CompanyId";
			field_exists = false;
			field_exists_or_not(text2, text);
			if (!field_exists)
			{
				string text3 = "ALTER TABLE " + text + " ADD COLUMN " + text2 + " long";
				cmd = new OleDbCommand(text3, con);
				cmd.ExecuteNonQuery();
				text3 = "Update " + text + " set " + text2 + "=0";
				cmd = new OleDbCommand(text3, con);
				cmd.ExecuteNonQuery();
			}
			else
			{
				string text4 = "Update " + text + " set " + text2 + "=0";
				cmd = new OleDbCommand(text4, con);
				cmd.ExecuteNonQuery();
			}
			((Component)(object)con).Dispose();
			con.Close();
		}

		private void Count_selected_cmps()
		{
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
							int num3 = 0;
							int num4 = grid3.Rows.Count - 1;
							for (int i = 0; i <= num4 && !Conversions.ToBoolean(Operators.OrObject(Operators.CompareObjectEqual(grid3.Rows[i].Cells[1].Value, "", TextCompare: false), Operators.CompareObjectEqual(grid3.Rows[i].Cells[1].Value, null, TextCompare: false))); i++)
							{
								if (Operators.ConditionalCompareObjectEqual(grid3.Rows[i].Cells[0].Value, true, TextCompare: false))
								{
									num3++;
								}
							}
							TOT_selected_cmps = num3;
							lbl_sel_cmps.Text = ("Selected Firms : " + Conversions.ToString(num3) + " of " + Conversions.ToString(grid3.Rows.Count)) ?? "";
							break;
						}
						case 288:
							num = -1;
							switch (num2)
							{
							case 2:
								break;
							default:
								goto IL_0156;
							}
							break;
						}
					}
					catch (Exception ex) when ((num2 != 0) & (num == 0))
					{
						ProjectData.SetProjectError(ex);
						try0001_dispatch = 288;
						continue;
					}
					break;
					IL_0156:
					throw ProjectData.CreateProjectError(-2146828237);
				}
				if (num != 0)
				{
					ProjectData.ClearProjectError();
				}
			}
		}

		private void GET_companies()
		{
			//IL_011f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0126: Expected Obj, but got Unknown
			//IL_00b3: Unknown result type (might be due to invalid IL or missing references)
			//IL_00bd: Expected Obj, but got Unknown
			//IL_00d9: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e3: Expected Obj, but got Unknown
			DirectoryInfo directoryInfo = new DirectoryInfo(Module1.db_path);
			FileInfo[] files = directoryInfo.GetFiles();
			int num = 0;
			FileInfo[] array = files;
			checked
			{
				foreach (FileInfo fileInfo in array)
				{
					if (Versioned.IsNumeric(fileInfo.Extension) && Operators.CompareString(Strings.UCase(Strings.Left(fileInfo.Name, 4)), Strings.UCase("data"), TextCompare: false) == 0)
					{
						conn(fileInfo.FullName);
						string tbl_nm = "CompanyInfo";
						string field_nm = "SyncEnabled";
						field_exists = false;
						field_exists_or_not(field_nm, tbl_nm);
						if (!field_exists)
						{
							string text = "ALTER TABLE CompanyInfo ADD COLUMN SyncEnabled INTEGER";
							cmd = new OleDbCommand(text, con);
							cmd.ExecuteNonQuery();
							text = "Update CompanyInfo Set SyncEnabled=1";
							cmd = new OleDbCommand(text, con);
							cmd.ExecuteNonQuery();
						}
						get_comp_details_from_DB(fileInfo.FullName, num);
						num++;
					}
				}
				DataGridViewCheckBoxColumn val = new DataGridViewCheckBoxColumn();
				val.FlatStyle = (FlatStyle)2;
				grid3.Columns.Insert(0, (DataGridViewColumn)(object)val);
				grid3.Columns[0].HeaderText = "Select";
				int num2 = grid3.Rows.Count - 1;
				for (int j = 0; j <= num2 && !Operators.ConditionalCompareObjectEqual(grid3.Rows[j].Cells[1].Value, null, TextCompare: false); j++)
				{
					if (Conversion.Val(RuntimeHelpers.GetObjectValue(grid3.Rows[j].Cells[5].Value)) == 1.0)
					{
						grid3.Rows[j].Cells[0].Value = true;
					}
				}
			}
		}

		private void Comp_ID()
		{
			//IL_0097: Unknown result type (might be due to invalid IL or missing references)
			//IL_009e: Expected Obj, but got Unknown
			//IL_0117: Unknown result type (might be due to invalid IL or missing references)
			//IL_0121: Expected Obj, but got Unknown
			int try0001_dispatch = -1;
			int num3 = default;
			int num = default;
			int num2 = default;
			int num5 = default;
			int num6 = default;
			OleDbConnection val = default;
			string text = default;
			while (true)
			{
				try
				{
					/*Note: ILSpy has introduced the following switch to emulate a goto from catch-block to try-block*/;
					switch (try0001_dispatch)
					{
					default:
						ProjectData.ClearProjectError();
						num3 = -2;
						goto IL_000b;
					case 552:
						{
							num = num2;
							switch ((num3 <= -2) ? 1 : num3)
							{
							case 1:
								break;
							default:
								goto end_IL_0001;
							}
							int num4 = num + 1;
							num = 0;
							switch (num4)
							{
							case 1:
								break;
							case 2:
								goto IL_000b;
							case 3:
								goto IL_0028;
							case 5:
								goto IL_0095;
							case 6:
								goto IL_009e;
							case 7:
								goto IL_00b0;
							case 8:
								goto IL_00ba;
							case 9:
								goto IL_00fa;
							case 10:
								goto IL_0105;
							case 11:
								goto IL_010f;
							case 12:
								goto IL_0121;
							case 13:
								goto IL_012f;
							case 14:
								goto IL_0144;
							case 15:
								goto IL_016b;
							case 16:
							case 17:
								goto IL_01ba;
							default:
								goto end_IL_0001;
							case 4:
							case 18:
								goto end_IL_0001_2;
							}
							goto default;
						}
						IL_0144:
						num2 = 14;
						if (ds.Tables[0].Rows.Count > 0)
						{
							goto IL_016b;
						}
						goto IL_01ba;
						IL_016b:
						num2 = 15;
						grid3.Rows[num5].Cells[6].Value = ds.Tables[0].Rows[0][0].ToString();
						goto IL_01ba;
						IL_012f:
						num2 = 13;
						((DbDataAdapter)(object)da).Fill(ds);
						goto IL_0144;
						IL_01ba:
						num2 = 17;
						num5 = checked(num5 + 1);
						goto IL_01c3;
						IL_000b:
						num2 = 2;
						num6 = checked(grid3.Rows.Count - 1);
						num5 = 0;
						goto IL_01c3;
						IL_01c3:
						if (num5 > num6)
						{
							goto end_IL_0001_2;
						}
						goto IL_0028;
						IL_0028:
						num2 = 3;
						if (Conversions.ToBoolean(Operators.OrObject(Operators.CompareObjectEqual(grid3.Rows[num5].Cells[3].Value, "", TextCompare: false), Operators.CompareObjectEqual(grid3.Rows[num5].Cells[3].Value, null, TextCompare: false))))
						{
							goto end_IL_0001_2;
						}
						goto IL_0095;
						IL_0095:
						num2 = 5;
						val = new OleDbConnection();
						goto IL_009e;
						IL_009e:
						num2 = 6;
						if (val.State == ConnectionState.Open)
						{
							goto IL_00b0;
						}
						goto IL_00ba;
						IL_00b0:
						num2 = 7;
						val.Close();
						goto IL_00ba;
						IL_00ba:
						num2 = 8;
						val.ConnectionString = "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + grid3.Rows[num5].Cells[3].Value.ToString() + ";";
						goto IL_00fa;
						IL_00fa:
						num2 = 9;
						val.Open();
						goto IL_0105;
						IL_0105:
						num2 = 10;
						text = "SELECT CompanyId from OtherSettings";
						goto IL_010f;
						IL_010f:
						num2 = 11;
						da = new OleDbDataAdapter(text, val);
						goto IL_0121;
						IL_0121:
						num2 = 12;
						ds = new DataSet();
						goto IL_012f;
						end_IL_0001:
						break;
					}
				}
				catch (Exception ex) when ((num3 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 552;
					continue;
				}
				throw ProjectData.CreateProjectError(-2146828237);
				continue;
				end_IL_0001_2:
				break;
			}
			if (num != 0)
			{
				ProjectData.ClearProjectError();
			}
		}

		private void Get_TempLastEnteredVoucher()
		{
			//IL_0001: Unknown result type (might be due to invalid IL or missing references)
			//IL_0007: Expected Obj, but got Unknown
			//IL_002b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0035: Expected Obj, but got Unknown
			OleDbConnection val = new OleDbConnection();
			if (val.State == ConnectionState.Open)
			{
				val.Close();
			}
			val.Open();
			string text = "SELECT AccountingYearFrom,AccountingYearTo from TempLastEnteredVoucher";
			da = new OleDbDataAdapter(text, val);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			checked
			{
				if (ds.Tables[0].Rows.Count > 0)
				{
					DataGridView1.RowCount = (int)Math.Round(Conversion.Val(ds.Tables[0].Rows.Count) + 2.0);
					int num = ds.Tables[0].Rows.Count - 1;
					for (int i = 0; i <= num; i++)
					{
						DataGridView1.Rows[i].Cells[0].Value = ds.Tables[0].Rows[i][0].ToString();
						DataGridView1.Rows[i].Cells[1].Value = ds.Tables[0].Rows[i][1].ToString();
					}
				}
			}
		}

		private void get_comp_details_from_DB(string filenm, int row)
		{
			//IL_00a8: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ae: Expected Obj, but got Unknown
			//IL_006a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0074: Expected Obj, but got Unknown
			//IL_0090: Unknown result type (might be due to invalid IL or missing references)
			//IL_009a: Expected Obj, but got Unknown
			//IL_00f7: Unknown result type (might be due to invalid IL or missing references)
			//IL_0101: Expected Obj, but got Unknown
			//IL_0321: Unknown result type (might be due to invalid IL or missing references)
			//IL_032b: Expected Obj, but got Unknown
			conn(filenm);
			string text = "CompanyInfo";
			string text2 = "SyncEnabled";
			field_exists = false;
			field_exists_or_not(text2, text);
			if (!field_exists)
			{
				string text3 = "ALTER TABLE " + text + " ADD COLUMN " + text2 + " INTEGER";
				cmd = new OleDbCommand(text3, con);
				cmd.ExecuteNonQuery();
				text3 = "Update CompanyInfo Set SyncEnabled=1";
				cmd = new OleDbCommand(text3, con);
				cmd.ExecuteNonQuery();
			}
			OleDbConnection val = new OleDbConnection();
			if (val.State == ConnectionState.Open)
			{
				val.Close();
			}
			string startupPath = Application.StartupPath;
			val.ConnectionString = "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + filenm + ";";
			val.Open();
			string text4 = "SELECT CompanyName,AccYearFrom,AccYearTo,SyncEnabled from CompanyInfo";
			da = new OleDbDataAdapter(text4, val);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			if (ds.Tables[0].Rows.Count <= 0)
			{
				return;
			}
			checked
			{
				grid3.RowCount = (int)Math.Round(Conversion.Val(row) + 1.0);
				string value = ds.Tables[0].Rows[0][0].ToString();
				string text5 = Conversions.ToString(Conversions.ToDate(ds.Tables[0].Rows[0][1].ToString()));
				string text6 = Conversions.ToString(Conversions.ToDate(ds.Tables[0].Rows[ds.Tables[0].Rows.Count - 1][2].ToString()));
				string value2 = Strings.Mid(text5, text5.Length - 3, 4) + "-" + Strings.Mid(text6, text6.Length - 3, 4);
				grid3.Rows[row].Cells[0].Value = value;
				grid3.Rows[row].Cells[1].Value = value2;
				grid3.Rows[row].Cells[2].Value = filenm;
				grid3.Rows[row].Cells[4].Value = ds.Tables[0].Rows[0][3].ToString();
				grid3.Columns[4].Visible = false;
				text4 = "Select max(VoucherDate) from Transactions";
				da = new OleDbDataAdapter(text4, con);
				ds = new DataSet();
				((DbDataAdapter)(object)da).Fill(ds);
				dt = ds.Tables[0];
				if (dt.Rows.Count > 0)
				{
					if (Operators.CompareString(dt.Rows[0][0].ToString(), null, TextCompare: false) != 0)
					{
						grid3.Rows[row].Cells[3].Value = Conversions.ToDate(dt.Rows[0][0].ToString()).ToString("dd-MM-yyyy");
					}
					else
					{
						grid3.Rows[row].Cells[3].Value = "-------------";
					}
				}
				else
				{
					grid3.Rows[row].Cells[3].Value = "-------------";
				}
			}
		}

		private void Create_Table_Extra()
		{
			//IL_001d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0027: Expected Obj, but got Unknown
			//IL_008d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0097: Expected Obj, but got Unknown
			//IL_00c0: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ca: Expected Obj, but got Unknown
			int try0001_dispatch = -1;
			int num3 = default;
			int num = default;
			int num2 = default;
			string text = default;
			string text2 = default;
			string text3 = default;
			string[] array = default;
			DataTable schema = default;
			while (true)
			{
				try
				{
					/*Note: ILSpy has introduced the following switch to emulate a goto from catch-block to try-block*/;
					switch (try0001_dispatch)
					{
					default:
						ProjectData.ClearProjectError();
						num3 = -2;
						goto IL_000b;
					case 307:
						{
							num = num2;
							switch ((num3 <= -2) ? 1 : num3)
							{
							case 1:
								break;
							default:
								goto end_IL_0001;
							}
							int num4 = num + 1;
							num = 0;
							switch (num4)
							{
							case 1:
								break;
							case 2:
								goto IL_000b;
							case 3:
								goto IL_0013;
							case 4:
								goto IL_0027;
							case 5:
								goto IL_0035;
							case 6:
								goto IL_003f;
							case 7:
								goto IL_004a;
							case 8:
								goto IL_0060;
							case 9:
								goto IL_0077;
							case 10:
								goto IL_0081;
							case 11:
								goto IL_0097;
							case 13:
								goto IL_00aa;
							case 14:
								goto IL_00b4;
							case 15:
								goto end_IL_0001_2;
							default:
								goto end_IL_0001;
							case 12:
							case 16:
							case 17:
								goto end_IL_0001_3;
							}
							goto default;
						}
						IL_0097:
						num2 = 11;
						cmd.ExecuteNonQuery();
						goto end_IL_0001_3;
						IL_00aa:
						num2 = 13;
						text = "Delete from Extra";
						goto IL_00b4;
						IL_0081:
						num2 = 10;
						cmd = new OleDbCommand(text2, con);
						goto IL_0097;
						IL_00b4:
						num2 = 14;
						cmd = new OleDbCommand(text, con);
						break;
						IL_000b:
						num2 = 2;
						text3 = "DROP TABLE Extra";
						goto IL_0013;
						IL_0013:
						num2 = 3;
						cmd = new OleDbCommand(text3, con);
						goto IL_0027;
						IL_0027:
						num2 = 4;
						cmd.ExecuteNonQuery();
						goto IL_0035;
						IL_0035:
						num2 = 5;
						array = new string[4];
						goto IL_003f;
						IL_003f:
						num2 = 6;
						array[2] = "Extra";
						goto IL_004a;
						IL_004a:
						num2 = 7;
						schema = con.GetSchema("Tables", array);
						goto IL_0060;
						IL_0060:
						num2 = 8;
						if (schema.Rows.Count == 0)
						{
							goto IL_0077;
						}
						goto IL_00aa;
						IL_0077:
						num2 = 9;
						text2 = "CREATE TABLE Extra (ID COUNTER PRIMARY KEY, AccountingYearFrom Text,AccountingYearTo Text)";
						goto IL_0081;
						end_IL_0001_2:
						break;
					}
					num2 = 15;
					cmd.ExecuteNonQuery();
					break;
					end_IL_0001:;
				}
				catch (Exception ex) when ((num3 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 307;
					continue;
				}
				throw ProjectData.CreateProjectError(-2146828237);
				continue;
				end_IL_0001_3:
				break;
			}
			if (num != 0)
			{
				ProjectData.ClearProjectError();
			}
		}

		private void Button2_Click(object sender, EventArgs e)
		{
			//IL_000f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0019: Expected Obj, but got Unknown
			//IL_065b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0665: Expected Obj, but got Unknown
			string text = "SELECT StockTransactions.VoucherNumber,StockTransactions.TransType,StockTransactions.ItemCode, StockTransactions.Bags, StockTransactions.Rate, StockTransactions.Amount, StockItems.Code1st, StockItems.ItemName, StockUnits.UnitName FROM ((StockTransactions INNER JOIN StockItems  ON StockTransactions.ItemCode = StockItems.Code1st) INNER JOIN StockUnits ON StockItems.UnitCode = StockUnits.Code1st)WHERE StockTransactions.TransType = 'Sale' OR StockTransactions.TransType = 'Purc'";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			DataGridView1.ColumnCount = 3;
			checked
			{
				if (dt.Rows.Count > 0)
				{
					Application.DoEvents();
					pb1.Maximum = dt.Rows.Count + 2;
					int num = dt.Rows.Count - 1;
					for (int i = 0; i <= num; i++)
					{
						string text2 = "";
						int num2 = dt.Rows.Count - 1;
						for (int j = 0; j <= num2; j++)
						{
							if ((Operators.CompareString(dt.Rows[i][0].ToString(), dt.Rows[j][0].ToString(), TextCompare: false) == 0) & (Operators.CompareString(dt.Rows[i][1].ToString(), dt.Rows[j][1].ToString(), TextCompare: false) == 0))
							{
								if (Operators.CompareString(text2, "", TextCompare: false) == 0)
								{
									text2 = dt.Rows[j][7].ToString() + " " + dt.Rows[j][3].ToString() + " " + dt.Rows[j][8].ToString() + " @ " + dt.Rows[j][4].ToString();
									text2 = text2.Replace("None", "");
								}
								else
								{
									text2 = text2 + " " + dt.Rows[j][7].ToString() + "  " + dt.Rows[j][3].ToString() + dt.Rows[j][8].ToString() + " @ " + dt.Rows[j][4].ToString();
									text2 = text2.Replace("None", "");
								}
								text2 += "|";
							}
						}
						DataGridView1.RowCount += 1;
						DataGridView1.Rows[i].Cells[0].Value = dt.Rows[i][0].ToString();
						DataGridView1.Rows[i].Cells[1].Value = dt.Rows[i][1].ToString();
						DataGridView1.Rows[i].Cells[2].Value = text2;
						Application.DoEvents();
						pb1.Value = i + 1;
						Label4.Text = Conversions.ToString(Math.Ceiling(Conversion.Val(i) * 100.0 / Conversion.Val(dt.Rows.Count))) + " Stage3 % Completed";
						Thread.Sleep(1);
					}
				}
				int num3 = DataGridView1.Rows.Count - 1;
				for (int k = num3; k >= 0; k += -1)
				{
					int num4 = k - 1;
					for (int l = num4; l >= 0; l += -1)
					{
						if (Conversions.ToBoolean(Operators.AndObject(Operators.CompareObjectEqual(DataGridView1.Rows[k].Cells[0].Value, DataGridView1.Rows[l].Cells[0].Value, TextCompare: false), Operators.CompareObjectEqual(DataGridView1.Rows[k].Cells[1].Value, DataGridView1.Rows[l].Cells[1].Value, TextCompare: false))))
						{
							DataGridView1.Rows.RemoveAt(k);
							break;
						}
					}
				}
				Application.DoEvents();
				Create_Table();
				pb1.Value = 0;
				pb1.Maximum = DataGridView1.RowCount + 2;
				int num5 = DataGridView1.RowCount - 1;
				for (int m = 0; m <= num5; m++)
				{
					long num6 = Conversions.ToLong(DataGridView1.Rows[m].Cells[0].Value);
					string text3 = Conversions.ToString(DataGridView1.Rows[m].Cells[1].Value);
					string text4 = Conversions.ToString(DataGridView1.Rows[m].Cells[2].Value);
					string text5 = "insert into NewTable(VoucherNo,VoucherType, details) values (" + Conversions.ToString(num6) + ",'" + text3 + "','" + text4 + "')";
					cmd = new OleDbCommand(text5, con);
					cmd.ExecuteNonQuery();
					Application.DoEvents();
					pb1.Value = m + 1;
					Label1.Text = Label2.Text;
					Label4.Text = Conversions.ToString(Math.Ceiling(Conversion.Val(m) * 100.0 / Conversion.Val(dt.Rows.Count))) + " Final Stage % Completed";
					Thread.Sleep(1);
				}
				Button1_Click(RuntimeHelpers.GetObjectValue(sender), e);
			}
		}

		private void Create_Table()
		{
			//IL_001d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0027: Expected Obj, but got Unknown
			//IL_008d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0097: Expected Obj, but got Unknown
			//IL_00c0: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ca: Expected Obj, but got Unknown
			int try0001_dispatch = -1;
			int num3 = default;
			int num = default;
			int num2 = default;
			string text = default;
			string text2 = default;
			string text3 = default;
			string[] array = default;
			DataTable schema = default;
			while (true)
			{
				try
				{
					/*Note: ILSpy has introduced the following switch to emulate a goto from catch-block to try-block*/;
					switch (try0001_dispatch)
					{
					default:
						ProjectData.ClearProjectError();
						num3 = -2;
						goto IL_000b;
					case 307:
						{
							num = num2;
							switch ((num3 <= -2) ? 1 : num3)
							{
							case 1:
								break;
							default:
								goto end_IL_0001;
							}
							int num4 = num + 1;
							num = 0;
							switch (num4)
							{
							case 1:
								break;
							case 2:
								goto IL_000b;
							case 3:
								goto IL_0013;
							case 4:
								goto IL_0027;
							case 5:
								goto IL_0035;
							case 6:
								goto IL_003f;
							case 7:
								goto IL_004a;
							case 8:
								goto IL_0060;
							case 9:
								goto IL_0077;
							case 10:
								goto IL_0081;
							case 11:
								goto IL_0097;
							case 13:
								goto IL_00aa;
							case 14:
								goto IL_00b4;
							case 15:
								goto end_IL_0001_2;
							default:
								goto end_IL_0001;
							case 12:
							case 16:
							case 17:
								goto end_IL_0001_3;
							}
							goto default;
						}
						IL_0097:
						num2 = 11;
						cmd.ExecuteNonQuery();
						goto end_IL_0001_3;
						IL_00aa:
						num2 = 13;
						text = "Delete from NewTable";
						goto IL_00b4;
						IL_0081:
						num2 = 10;
						cmd = new OleDbCommand(text2, con);
						goto IL_0097;
						IL_00b4:
						num2 = 14;
						cmd = new OleDbCommand(text, con);
						break;
						IL_000b:
						num2 = 2;
						text3 = "DROP TABLE NewTable";
						goto IL_0013;
						IL_0013:
						num2 = 3;
						cmd = new OleDbCommand(text3, con);
						goto IL_0027;
						IL_0027:
						num2 = 4;
						cmd.ExecuteNonQuery();
						goto IL_0035;
						IL_0035:
						num2 = 5;
						array = new string[4];
						goto IL_003f;
						IL_003f:
						num2 = 6;
						array[2] = "NewTable";
						goto IL_004a;
						IL_004a:
						num2 = 7;
						schema = con.GetSchema("Tables", array);
						goto IL_0060;
						IL_0060:
						num2 = 8;
						if (schema.Rows.Count == 0)
						{
							goto IL_0077;
						}
						goto IL_00aa;
						IL_0077:
						num2 = 9;
						text2 = "CREATE TABLE NewTable (ID COUNTER PRIMARY KEY, VoucherNo INTEGER,VoucherType char,details TEXT,vdate datetime)";
						goto IL_0081;
						end_IL_0001_2:
						break;
					}
					num2 = 15;
					cmd.ExecuteNonQuery();
					break;
					end_IL_0001:;
				}
				catch (Exception ex) when ((num3 != 0) & (num == 0))
				{
					ProjectData.SetProjectError(ex);
					try0001_dispatch = 307;
					continue;
				}
				throw ProjectData.CreateProjectError(-2146828237);
				continue;
				end_IL_0001_3:
				break;
			}
			if (num != 0)
			{
				ProjectData.ClearProjectError();
			}
		}

		public string ConvertDataTabletoString()
		{
			//IL_000d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0013: Expected Obj, but got Unknown
			//IL_001a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0020: Expected Obj, but got Unknown
			//IL_0028: Unknown result type (might be due to invalid IL or missing references)
			//IL_002f: Expected Obj, but got Unknown
			//IL_0038: Unknown result type (might be due to invalid IL or missing references)
			//IL_003f: Expected Obj, but got Unknown
			DataTable dataTable = new DataTable();
			SqlConnection val = new SqlConnection("Data Source=SureshDasari;Initial Catalog=master;Integrated Security=true");
			try
			{
				SqlCommand val2 = new SqlCommand("select title=City,lat=latitude,lng=longitude,description from LocationDetails", val);
				try
				{
					val.Open();
					SqlDataAdapter val3 = new SqlDataAdapter(val2);
					((DbDataAdapter)(object)val3).Fill(dataTable);
					JavaScriptSerializer val4 = new JavaScriptSerializer();
					List<Dictionary<string, object>> list = new List<Dictionary<string, object>>();
					foreach (DataRow row in dataTable.Rows)
					{
						Dictionary<string, object> dictionary = new Dictionary<string, object>();
						foreach (DataColumn column in dataTable.Columns)
						{
							dictionary.Add(column.ColumnName, RuntimeHelpers.GetObjectValue(row[column]));
						}
						list.Add(dictionary);
					}
					return val4.Serialize((object)list);
				}
				finally
				{
					((IDisposable)val2)?.Dispose();
				}
			}
			finally
			{
				((IDisposable)val)?.Dispose();
			}
		}

		public static string GetJson1(DataTable dt)
		{
			//IL_0001: Unknown result type (might be due to invalid IL or missing references)
			//IL_0007: Expected Obj, but got Unknown
			JavaScriptSerializer val = new JavaScriptSerializer();
			val.MaxJsonLength = int.MaxValue;
			List<Dictionary<string, object>> list = new List<Dictionary<string, object>>();
			Dictionary<string, object> dictionary = null;
			foreach (DataRow row in dt.Rows)
			{
				dictionary = new Dictionary<string, object>();
				foreach (DataColumn column in dt.Columns)
				{
					dictionary.Add(column.ColumnName.Trim(), RuntimeHelpers.GetObjectValue(row[column]));
				}
				list.Add(dictionary);
			}
			return val.Serialize((object)list);
		}

		private void GET_FTP_Crediential()
		{
			string args = "{\"mobileno\":\"" + Module1.user_mobile + "\"}";
			string url = "http://kamrasoftwares.com/bahikhata/web-service/index.php?service=getftp";
			upload_data_FTP_Cred(url, args);
		}

		private void upload_data_FTP_Cred(string url, string args)
		{
			try
			{
				Uri uri = new Uri(url);
				byte[] bytes = Encoding.UTF8.GetBytes(args);
				string text = SendRequest(uri, bytes, "application/json", "POST");
				JObject jObject = default;
				if (Operators.CompareString(text, "", TextCompare: false) != 0)
				{
					jObject = JObject.Parse(text);
				}
				if (Operators.CompareString(jObject["success"].ToString(), "1", TextCompare: false) == 0)
				{
					ftp_username = jObject["data"]["ftp_username"].ToString();
					ftp_username = ftp_username.Replace("\"", "");
					ftp_password = jObject["data"]["ftp_password"].ToString();
					ftp_password = ftp_password.Replace("\"", "");
					ftp_remote_host = jObject["data"]["ftp_remote_host"].ToString();
					ftp_remote_host = ftp_remote_host.Replace("\"", "");
				}
			}
			catch (Exception ex)
			{
				ProjectData.SetProjectError(ex);
				Exception ex2 = ex;
				Interaction.MsgBox("Error Occured");
				ProjectData.ClearProjectError();
			}
		}

		private void Button3_Click(object sender, EventArgs e)
		{
			Count_selected_cmps();
			if (Module1.CheckForInternetConnection())
			{
				((Control)this).Cursor = CreateCursor(cursor_file);
				string text = "pc";
				string user_mobile = Module1.user_mobile;
				string user_email = Module1.user_email;
				string args = "{\"mobileno\":\"" + user_mobile + "\",\"email\":\"" + user_email + "\",\"pc\":\"" + text + "\"}";
				string url = Module1.URL + "web-service/index.php?service=syncpc";
				upload_data_1(url, args);
				if (Operators.CompareString(Module1.userid, "", TextCompare: false) == 0)
				{
					Interaction.MsgBox("User id is blank", MsgBoxStyle.Information, Module1.Title_msg);
					Application.Exit();
				}
				Process_All_Files();
				((Control)this).Cursor = Cursors.Default;
				Timer1.Enabled = false;
				((ButtonBase)Button3).Text = "Process Complete";
				Interaction.MsgBox("Synchronization Process Done", MsgBoxStyle.Information, Module1.Title_msg);
				((ButtonBase)Button3).Text = "Start Sync Data";
				((ButtonBase)Button3).BackColor = Color.FromArgb(225, 255, 255);
				pb1.Value = 0;
				((Control)pb1).Visible = false;
				if (con.State == ConnectionState.Open)
				{
					con.Close();
				}
				Application.Exit();
			}
			else
			{
				Interaction.MsgBox("Please Check your internet connection", MsgBoxStyle.Information, Module1.Title_msg);
			}
		}

		[DllImport("user32.dll", CharSet = CharSet.Unicode, EntryPoint = "LoadCursorFromFileW", ExactSpelling = true, SetLastError = true)]
		private static extern IntPtr LoadCursorFromFile([MarshalAs(UnmanagedType.VBByRefStr)] ref string filename);

		private static Cursor CreateCursor(string filename)
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

		private void upload_data_1(string url, string args)
		{
			Uri uri = new Uri(url);
			byte[] bytes = Encoding.UTF8.GetBytes(args);
			string text = SendRequest(uri, bytes, "application/json", "POST");
			JObject jObject = default;
			if (Operators.CompareString(text, "", TextCompare: false) != 0)
			{
				jObject = JObject.Parse(text);
			}
			if (Operators.CompareString(jObject["success"].ToString(), "1", TextCompare: false) == 0)
			{
				Module1.userid = jObject["data"]["userid"].ToString();
				Module1.userid = Module1.userid.Replace("\"", "");
			}
		}

		private void upload_data_2(string url, string args)
		{
			Uri uri = new Uri(url);
			byte[] bytes = Encoding.UTF8.GetBytes(args);
			string text = SendRequest(uri, bytes, "application/json", "POST");
			JObject jObject = default;
			if (Operators.CompareString(text, "", TextCompare: false) != 0)
			{
				jObject = JObject.Parse(text);
			}
			if (Operators.CompareString(jObject["success"].ToString(), "1", TextCompare: false) == 0)
			{
				company_id = jObject["company_id"].ToString();
				file1 = jObject["file_name"].ToString();
				file1 = file1.Replace("\"", "");
				company_id = company_id.Replace("\"", "");
			}
		}

		private void upload_data_3(string url, string args)
		{
			Uri uri = new Uri(url);
			byte[] bytes = Encoding.UTF8.GetBytes(args);
			string text = SendRequest(uri, bytes, "application/json", "POST");
			JObject jObject = default;
			if (Operators.CompareString(text, "", TextCompare: false) != 0)
			{
				jObject = JObject.Parse(text);
			}
			if (Operators.CompareString(jObject["success"].ToString(), "0", TextCompare: false) == 0)
			{
				Interaction.MsgBox(jObject["msg"].ToString(), MsgBoxStyle.OkOnly, Module1.Title_msg);
				Application.Exit();
				return;
			}
			company_id = jObject["company_id"].ToString();
			file1 = jObject["file_name"].ToString();
			file1 = file1.Replace("\"", "");
			company_id = company_id.Replace("\"", "");
		}

		private void upload_data_4(string url, string args)
		{
			Uri uri = new Uri(url);
			byte[] bytes = Encoding.UTF8.GetBytes(args);
			string text = SendRequest(uri, bytes, "application/json", "POST");
			JObject jObject = default;
			if (Operators.CompareString(text, "", TextCompare: false) != 0)
			{
				jObject = JObject.Parse(text);
			}
			if (Operators.CompareString(jObject["success"].ToString(), "0", TextCompare: false) == 0)
			{
				Interaction.MsgBox(jObject["msg"].ToString(), MsgBoxStyle.OkOnly, Module1.Title_msg);
				Application.Exit();
				return;
			}
			file1 = jObject["file_name"].ToString();
			file1 = file1.Replace("\"", "");
			if (Operators.CompareString(file1, "", TextCompare: false) == 0)
			{
				Interaction.MsgBox("File Name Not Found", MsgBoxStyle.Critical);
			}
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

		private void Button4_Click(object sender, EventArgs e)
		{
			//IL_00b7: Unknown result type (might be due to invalid IL or missing references)
			//IL_00c1: Expected Obj, but got Unknown
			//IL_01f9: Unknown result type (might be due to invalid IL or missing references)
			//IL_0203: Expected Obj, but got Unknown
			int num = 1;
			checked
			{
				do
				{
					string text;
					if (Strings.Len(num.ToString()) == 1)
					{
						text = "data.00" + Conversions.ToString(num);
					}
					else
					{
						text = ((Strings.Len(num.ToString()) != 2) ? ("data." + Conversions.ToString(num)) : ("data.0" + Conversions.ToString(num)));
					}
					if (File.Exists("D:\\Bahi-Khata\\DATA\\" + text))
					{
						conn("D:\\Bahi-Khata\\DATA\\" + text);
						Label2.Text = text;
						Thread.Sleep(1000);
						string text2 = "Select CompanyName,AccYearFrom,AccYearTo from CompanyInfo";
						da = new OleDbDataAdapter(text2, con);
						ds = new DataSet();
						((DbDataAdapter)(object)da).Fill(ds);
						dt = ds.Tables[0];
						DataGridView1.ColumnCount = 3;
						if (dt.Rows.Count > 0)
						{
							company_name = dt.Rows[0]["CompanyName"].ToString();
							string text3 = dt.Rows[0]["AccYearFrom"].ToString();
							string value = dt.Rows[dt.Rows.Count - 1]["AccYearTo"].ToString();
							text3 = Conversions.ToString(Conversions.ToDate(text3.ToString()).Year);
							value = Conversions.ToString(Conversions.ToDate(value).Year);
							firmshort_name = text;
							year = text3 + "-" + value;
						}
						text2 = "Select max(VoucherDate) from Transactions";
						da = new OleDbDataAdapter(text2, con);
						ds = new DataSet();
						((DbDataAdapter)(object)da).Fill(ds);
						dt = ds.Tables[0];
						DataGridView1.ColumnCount = 3;
						if (dt.Rows.Count > 0)
						{
							max_vch_date = dt.Rows[0][0].ToString();
						}
						string url = Module1.URL + "web-service/index.php?service=Company";
						string text4 = "add";
						string text5 = "pc";
						string args = "{\"option\":\"" + text4 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + text + "\",\"pc\":\"" + text5 + "\"}";
						upload_data_2(url, args);
						text4 = "update";
						args = string.Concat(new string[8]
						{
							string.Concat("{\"option\":\"" + text4 + "\",\"userid\":\"" + Module1.userid + "\",\"company_id\":", company_id),
							",\"company\":\"",
							company_name,
							"\",\"max_vch_date\":\"",
							max_vch_date,
							"\",\"pc\":\"",
							text5,
							"\"}"
						});
						string args2 = string.Concat(new string[8]
						{
							string.Concat("{\"option\":\"" + text4 + "\",\"userid\":\"" + Module1.userid + "\",\"company_id\":", company_id),
							",\"company\":\"",
							company_name,
							"\",\"max_vch_date\":\"",
							max_vch_date,
							"\",\"pc\":\"",
							text5,
							"\"}"
						});
						if (Operators.CompareString(company_id, "", TextCompare: false) != 0)
						{
							process_finish = false;
							btn_Fetch_data_Click(RuntimeHelpers.GetObjectValue(sender), e);
							while (!process_finish)
							{
								Application.DoEvents();
							}
							upload_data_3(url, args2);
						}
					}
					num++;
				}
				while (num <= 999);
			}
		}

		private void Process_All_Files1()
		{
			//IL_0140: Unknown result type (might be due to invalid IL or missing references)
			//IL_014a: Expected Obj, but got Unknown
			//IL_0282: Unknown result type (might be due to invalid IL or missing references)
			//IL_028c: Expected Obj, but got Unknown
			checked
			{
				int num = grid3.Rows.Count - 1;
				for (int i = 0; i <= num && !Operators.ConditionalCompareObjectEqual(grid3.Rows[i].Cells[0].Value, "", TextCompare: false); i++)
				{
					string text;
					if (Strings.Len(i.ToString()) == 1)
					{
						text = "data.00" + Conversions.ToString(i);
					}
					else if (Strings.Len(i.ToString()) == 2)
					{
						text = "data.0" + Conversions.ToString(i);
					}
					else
					{
						text = "data." + Conversions.ToString(i);
					}
					text = Conversions.ToString(grid3.Rows[i].Cells[2].Value);
					if (!File.Exists(text))
					{
						continue;
					}
					conn(text);
					Label2.Text = Conversions.ToString(grid3.Rows[i].Cells[0].Value);
					Thread.Sleep(1000);
					string text2 = "Select CompanyName,AccYearFrom,AccYearTo from CompanyInfo";
					da = new OleDbDataAdapter(text2, con);
					ds = new DataSet();
					((DbDataAdapter)(object)da).Fill(ds);
					dt = ds.Tables[0];
					DataGridView1.ColumnCount = 3;
					if (dt.Rows.Count > 0)
					{
						company_name = dt.Rows[0]["CompanyName"].ToString();
						string text3 = dt.Rows[0]["AccYearFrom"].ToString();
						string value = dt.Rows[dt.Rows.Count - 1]["AccYearTo"].ToString();
						text3 = Conversions.ToString(Conversions.ToDate(text3.ToString()).Year);
						value = Conversions.ToString(Conversions.ToDate(value).Year);
						firmshort_name = text;
						year = text3 + "-" + value;
					}
					text2 = "Select max(VoucherDate) from Transactions";
					da = new OleDbDataAdapter(text2, con);
					ds = new DataSet();
					((DbDataAdapter)(object)da).Fill(ds);
					dt = ds.Tables[0];
					DataGridView1.ColumnCount = 3;
					if (dt.Rows.Count <= 0)
					{
						continue;
					}
					max_vch_date = dt.Rows[0][0].ToString();
					if (Operators.CompareString(max_vch_date, "", TextCompare: false) == 0)
					{
						continue;
					}
					string url = Module1.URL + "web-service/index.php?service=Company";
					string text4 = "add";
					string text5 = "pc";
					DateTime lastWriteTime = File.GetLastWriteTime(text);
					string args = "{\"option\": \"" + text4 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + text + "\",\"pc\":\"" + text5 + "\"}";
					upload_data_2(url, args);
					text4 = "update";
					args = "{\"option\":\"" + text4 + "\",\"userid\":\"" + Module1.userid + "\",\"company_id\":\"" + company_id + "\",\"company\":\"" + company_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"pc\":\"" + text5 + "\"}";
					if (Operators.CompareString(company_id, "", TextCompare: false) != 0)
					{
						process_finish = false;
						fetch_all_data();
						while (!process_finish)
						{
							Application.DoEvents();
						}
						upload_data_3(url, args);
					}
					else
					{
						Interaction.MsgBox("Some Issue from Cloud side", MsgBoxStyle.OkOnly, Module1.Title_msg);
						Application.Exit();
					}
				}
			}
		}

		private void Process_All_Files()
		{
			//IL_01ef: Unknown result type (might be due to invalid IL or missing references)
			//IL_01f9: Expected Obj, but got Unknown
			//IL_0361: Unknown result type (might be due to invalid IL or missing references)
			//IL_036b: Expected Obj, but got Unknown
			//IL_04b2: Unknown result type (might be due to invalid IL or missing references)
			//IL_04bc: Expected Obj, but got Unknown
			//IL_0668: Unknown result type (might be due to invalid IL or missing references)
			//IL_0672: Expected Obj, but got Unknown
			//IL_061a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0624: Expected Obj, but got Unknown
			//IL_0640: Unknown result type (might be due to invalid IL or missing references)
			//IL_064a: Expected Obj, but got Unknown
			//IL_08a5: Unknown result type (might be due to invalid IL or missing references)
			//IL_08af: Expected Obj, but got Unknown
			//IL_07ce: Unknown result type (might be due to invalid IL or missing references)
			//IL_07d8: Expected Obj, but got Unknown
			//IL_0947: Unknown result type (might be due to invalid IL or missing references)
			//IL_0951: Expected Obj, but got Unknown
			//IL_12b2: Unknown result type (might be due to invalid IL or missing references)
			//IL_12bc: Expected Obj, but got Unknown
			int num = 0;
			checked
			{
				int num2 = grid3.Rows.Count - 1;
				string text9 = default;
				string text10 = default;
				string text11 = default;
				for (int i = 0; i <= num2 && !Operators.ConditionalCompareObjectEqual(grid3.Rows[i].Cells[1].Value, "", TextCompare: false); i++)
				{
					if (!Operators.ConditionalCompareObjectEqual(grid3.Rows[i].Cells[0].Value, true, TextCompare: false))
					{
						continue;
					}
					num++;
					Label1.Text = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject(Operators.ConcatenateObject(grid3.Rows[i].Cells[1].Value, " ("), grid3.Rows[i].Cells[2].Value), ")"));
					Thread.Sleep(100);
					string text = Conversions.ToString(grid3.Rows[i].Cells[3].Value);
					if (!File.Exists(text))
					{
						continue;
					}
					conn(text);
					Label2.Text = Conversions.ToString(grid3.Rows[i].Cells[1].Value);
					Label2.Text = "Uploading " + Conversions.ToString(num) + " of " + Conversions.ToString(TOT_selected_cmps);
					((ButtonBase)Button3).BackColor = Color.FromArgb(0, 192, 0);
					((ButtonBase)Button3).Text = Label2.Text;
					Application.DoEvents();
					Thread.Sleep(1000);
					string text2 = "Select CompanyName,AccYearFrom,AccYearTo,MyState from CompanyInfo";
					da = new OleDbDataAdapter(text2, con);
					ds = new DataSet();
					((DbDataAdapter)(object)da).Fill(ds);
					dt = ds.Tables[0];
					if (dt.Rows.Count > 0)
					{
						company_name = dt.Rows[0]["CompanyName"].ToString();
						string text3 = dt.Rows[0]["AccYearFrom"].ToString();
						string value = dt.Rows[dt.Rows.Count - 1]["AccYearTo"].ToString();
						text3 = Conversions.ToString(Conversions.ToDate(text3.ToString()).Year);
						value = Conversions.ToString(Conversions.ToDate(value).Year);
						firmshort_name = Path.GetFileName(text);
						firm_state = dt.Rows[dt.Rows.Count - 1]["MyState"].ToString();
						year = text3 + "-" + value;
					}
					text2 = "Select max(VoucherDate) from Transactions";
					da = new OleDbDataAdapter(text2, con);
					ds = new DataSet();
					((DbDataAdapter)(object)da).Fill(ds);
					dt = ds.Tables[0];
					if (dt.Rows.Count > 0)
					{
						max_vch_date = dt.Rows[0][0].ToString();
						if (Operators.CompareString(max_vch_date, "", TextCompare: false) != 0)
						{
							max_vch_date = Conversions.ToDate(max_vch_date).ToString("yyyy-MM-dd");
						}
						else
						{
							max_vch_date = "------";
						}
					}
					else
					{
						max_vch_date = "------";
					}
					string url = Module1.URL + "web-service/index.php?service=Company";
					string text4 = "pc";
					string text5 = "OtherSettings";
					string text6 = "CompanyId";
					field_exists = false;
					field_exists_or_not(text6, text5);
					if (!field_exists)
					{
						text2 = "ALTER TABLE " + text5 + " ADD COLUMN " + text6 + " long";
						cmd = new OleDbCommand(text2, con);
						cmd.ExecuteNonQuery();
					}
					string txt_file_path = Module1.db_path + "\\" + firmshort_name + ".txt";
					string text7 = Read_text_file(txt_file_path);
					DateTime lastWriteTime = File.GetLastWriteTime(text);
					if (Operators.CompareString(text7, "", TextCompare: false) != 0 && Information.IsDate(text7))
					{
						DateTime dateTime = Conversions.ToDate(Convert.ToDateTime(text7).ToString("dd-MMM-yyyy h:mm:ss tt"));
						lastWriteTime = Conversions.ToDate(lastWriteTime.ToString("dd-MMM-yyyy h:mm:ss tt"));
						int num3 = Conversions.ToInteger(DateAndTime.DateDiff("s", dateTime, lastWriteTime).ToString());
						if (!CHKRewrite.Checked && num3 == 0)
						{
							continue;
						}
					}
					conn(text);
					text5 = "VoucherSettings";
					text6 = "AppOtherMobNo";
					field_exists = false;
					field_exists_or_not(text6, text5);
					if (!field_exists)
					{
						text2 = "ALTER TABLE " + text5 + " ADD COLUMN " + text6 + " text";
						cmd = new OleDbCommand(text2, con);
						cmd.ExecuteNonQuery();
						string text8 = "Update VoucherSettings set AppOtherMobNo=''";
						cmd = new OleDbCommand(text8, con);
						cmd.ExecuteNonQuery();
					}
					text2 = "SELECT AccountingYearFrom,AccountingYearTo from TempLastEnteredVoucher";
					da = new OleDbDataAdapter(text2, con);
					ds = new DataSet();
					((DbDataAdapter)(object)da).Fill(ds);
					if (ds.Tables[0].Rows.Count > 0)
					{
						text9 = ds.Tables[0].Rows[0][0].ToString();
						text10 = ds.Tables[0].Rows[0][1].ToString();
						if (Information.IsDate(text9))
						{
							text9 = Conversions.ToDate(ds.Tables[0].Rows[0][0].ToString()).ToString("yyyy-MM-dd");
						}
						if (Information.IsDate(text10))
						{
							text10 = Conversions.ToDate(ds.Tables[0].Rows[0][1].ToString()).ToString("yyyy-MM-dd");
						}
					}
					if (!Information.IsDate(text9) | !Information.IsDate(text10))
					{
						text2 = "SELECT Max(BooksBeginingFrom),Max(AccYearTo) from CompanyInfo";
						da = new OleDbDataAdapter(text2, con);
						ds = new DataSet();
						((DbDataAdapter)(object)da).Fill(ds);
						if (ds.Tables[0].Rows.Count > 0)
						{
							text9 = Conversions.ToDate(ds.Tables[0].Rows[0][0].ToString()).ToString("yyyy-MM-dd");
							text10 = Conversions.ToDate(ds.Tables[0].Rows[0][1].ToString()).ToString("yyyy-MM-dd");
						}
					}
					text2 = "SELECT AppOtherMobNo from VoucherSettings";
					da = new OleDbDataAdapter(text2, con);
					ds = new DataSet();
					((DbDataAdapter)(object)da).Fill(ds);
					if (ds.Tables[0].Rows.Count > 0)
					{
						text11 = ds.Tables[0].Rows[0][0].ToString();
					}
					if (Strings.Len(text11) > 0)
					{
						text11 = DecryptString(text11);
					}
					text2 = "SELECT CompanyId from otherSettings";
					da = new OleDbDataAdapter(text2, con);
					ds = new DataSet();
					((DbDataAdapter)(object)da).Fill(ds);
					if (ds.Tables[0].Rows.Count <= 0)
					{
						continue;
					}
					if (Conversion.Val(ds.Tables[0].Rows[0].ItemArray[0].ToString()) == 0.0)
					{
						string text12 = "add";
						string args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"numpass\":\"" + text11 + "\",\"accountingyearfrom\":\"" + text9 + "\",\"accountingyearto\":\"" + text10 + "\",\"gstin\":\"" + GSTIN + "\",\"pc\":\"" + text4 + "\"}";
						upload_data_2(url, args);
						if (Conversion.Val(company_id) == 0.0)
						{
							text12 = "add";
							args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"numpass\":\"" + text11 + "\",\"pc\":\"" + text4 + "\"}";
							args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"numpass\":\"" + text11 + "\",\"accountingyearfrom\":\"" + text9 + "\",\"accountingyearto\":\"" + text10 + "\",\"gstin\":\"" + GSTIN + "\",\"pc\":\"" + text4 + "\"}";
							upload_data_2(url, args);
						}
						if (Conversion.Val(company_id) == 0.0)
						{
							text12 = "add";
							args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"numpass\":\"" + text11 + "\",\"pc\":\"" + text4 + "\"}";
							upload_data_2(url, args);
						}
						if (Conversion.Val(company_id) == 0.0)
						{
							text12 = "add";
							args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"numpass\":\"" + text11 + "\",\"pc\":\"" + text4 + "\"}";
							args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"numpass\":\"" + text11 + "\",\"accountingyearfrom\":\"" + text9 + "\",\"accountingyearto\":\"" + text10 + "\",\"gstin\":\"" + GSTIN + "\",\"pc\":\"" + text4 + "\"}";
							upload_data_2(url, args);
						}
						if (Conversion.Val(company_id) == 0.0)
						{
							text12 = "add";
							args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"pc\":\"" + text4 + "\"}";
							args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"numpass\":\"" + text11 + "\",\"accountingyearfrom\":\"" + text9 + "\",\"accountingyearto\":\"" + text10 + "\",\"gstin\":\"" + GSTIN + "\",\"pc\":\"" + text4 + "\"}";
							upload_data_2(url, args);
						}
						if (Conversion.Val(company_id) == 0.0)
						{
							text12 = "add";
							args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"numpass\":\"" + text11 + "\",\"pc\":\"" + text4 + "\"}";
							args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"numpass\":\"" + text11 + "\",\"accountingyearfrom\":\"" + text9 + "\",\"accountingyearto\":\"" + text10 + "\",\"gstin\":\"" + GSTIN + "\",\"pc\":\"" + text4 + "\"}";
							upload_data_2(url, args);
						}
						conn(text);
						text2 = ("UPDATE OtherSettings SET CompanyId=" + company_id) ?? "";
						cmd = new OleDbCommand(text2, con);
						cmd.ExecuteNonQuery();
						process_finish = false;
						fetch_all_data();
						while (!process_finish)
						{
							Application.DoEvents();
						}
						if (con.State == ConnectionState.Open)
						{
							con.Close();
						}
						Thread.Sleep(1000);
						DateTime dateTime2 = Conversions.ToDate(File.GetLastWriteTime(text).ToString("dd-MMM-yyyy h:mm:ss tt"));
						Write_txt_file(Conversions.ToString(dateTime2), txt_file_path);
					}
					else
					{
						string text13 = ds.Tables[0].Rows[0].ItemArray[0].ToString();
						string text12 = "getfilename";
						string args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company_id\":\"" + text13 + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"numpass\":\"" + text11 + "\",\"accountingyearfrom\":\"" + text9 + "\",\"accountingyearto\":\"" + text10 + "\",\"gstin\":\"" + GSTIN + "\",\"pc\":\"" + text4 + "\"}";
						upload_data_4(url, args);
						process_finish = false;
						fetch_all_data();
						while (!process_finish)
						{
							Application.DoEvents();
						}
						text12 = "update";
						args = "{\"option\":\"" + text12 + "\",\"userid\":\"" + Module1.userid + "\",\"company_id\":\"" + text13 + "\",\"company\":\"" + company_name + "\",\"firmshort_name\":\"" + firmshort_name + "\",\"max_vch_date\":\"" + max_vch_date + "\",\"year\":\"" + year + "\",\"file\":\"" + firmshort_name + "\",\"numpass\":\"" + text11 + "\",\"accountingyearfrom\":\"" + text9 + "\",\"accountingyearto\":\"" + text10 + "\",\"gstin\":\"" + GSTIN + "\",\"pc\":\"" + text4 + "\"}";
						upload_data_3(url, args);
						if (con.State == ConnectionState.Open)
						{
							con.Close();
						}
						Thread.Sleep(1000);
						DateTime dateTime3 = Conversions.ToDate(File.GetLastWriteTime(text).ToString("dd-MMM-yyyy h:mm:ss tt"));
						Write_txt_file(Conversions.ToString(dateTime3), txt_file_path);
					}
				}
			}
		}

		public string DecryptString(string valToDecrpt)
		{
			string text = valToDecrpt;
			int num = Strings.Asc(Strings.Left(text, 1));
			int num2 = Strings.Asc(Strings.Right(text, 1));
			checked
			{
				text = Strings.Mid(text, 2, Strings.Len(text) - 2);
				int num3 = Strings.Len(text);
				string text2 = default;
				for (int i = 1; i <= num3; i++)
				{
					text2 = Conversions.ToString(Strings.Chr(Strings.Asc(Strings.Mid(text, i, 1)) - num + num2)) + text2;
				}
				return text2;
			}
		}

		private void Button5_Click(object sender, EventArgs e)
		{
			string text = "D:\\a.zip";
			Package val = Package.Open(text, FileMode.Create, FileAccess.ReadWrite);
			string text2 = "D:\\MyTest1.txt";
			string path = text2.Replace(" ", "_");
			string uriString = "/" + Path.GetFileName(path);
			Uri uri = new Uri(uriString, UriKind.Relative);
			string text3 = "";
			PackagePart val2 = val.CreatePart(uri, "", (CompressionOption)0);
			byte[] array = File.ReadAllBytes(text2);
			val2.GetStream().Write(array, 0, array.Length);
			val.Close();
		}

		private void CreateZipFile(string source, string destination)
		{
			Package val = Package.Open(destination, FileMode.Create, FileAccess.ReadWrite);
			string path = source.Replace(" ", "_");
			string uriString = "/" + Path.GetFileName(path);
			Uri uri = new Uri(uriString, UriKind.Relative);
			string text = "";
			PackagePart val2 = val.CreatePart(uri, "", (CompressionOption)0);
			byte[] array = File.ReadAllBytes(source);
			val2.GetStream().Write(array, 0, array.Length);
			val.Close();
		}

		private void Button6_Click(object sender, EventArgs e)
		{
		}

		private void pb1_Click(object sender, EventArgs e)
		{
		}

		private void btn_add_fields_Click(object sender, EventArgs e)
		{
			//IL_0057: Unknown result type (might be due to invalid IL or missing references)
			//IL_0061: Expected Obj, but got Unknown
			//IL_00d0: Unknown result type (might be due to invalid IL or missing references)
			//IL_00da: Expected Obj, but got Unknown
			string text = "Groups";
			string text2 = "SyncEnabled";
			field_exists_or_not(text2, text);
			if (!field_exists)
			{
				string text3 = "ALTER TABLE " + text + " ADD COLUMN " + text2 + " BIT";
				cmd = new OleDbCommand(text3, con);
				cmd.ExecuteNonQuery();
			}
			text = "Ledgers";
			text2 = "SyncEnabled";
			field_exists = false;
			field_exists_or_not(text2, text);
			if (!field_exists)
			{
				string text4 = "ALTER TABLE " + text + " ADD COLUMN " + text2 + " BIT";
				cmd = new OleDbCommand(text4, con);
				cmd.ExecuteNonQuery();
			}
		}

		private void Add_fields()
		{
			//IL_0061: Unknown result type (might be due to invalid IL or missing references)
			//IL_006b: Expected Obj, but got Unknown
			//IL_0085: Unknown result type (might be due to invalid IL or missing references)
			//IL_008f: Expected Obj, but got Unknown
			//IL_00a9: Unknown result type (might be due to invalid IL or missing references)
			//IL_00b3: Expected Obj, but got Unknown
			//IL_0125: Unknown result type (might be due to invalid IL or missing references)
			//IL_012f: Expected Obj, but got Unknown
			//IL_014b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0155: Expected Obj, but got Unknown
			//IL_0171: Unknown result type (might be due to invalid IL or missing references)
			//IL_017b: Expected Obj, but got Unknown
			//IL_01ea: Unknown result type (might be due to invalid IL or missing references)
			//IL_01f4: Expected Obj, but got Unknown
			//IL_0210: Unknown result type (might be due to invalid IL or missing references)
			//IL_021a: Expected Obj, but got Unknown
			string text = "Groups";
			string text2 = "SyncEnabled";
			field_exists = false;
			field_exists_or_not(text2, text);
			if (!field_exists)
			{
				string text3 = "ALTER TABLE " + text + " ADD COLUMN " + text2 + " INTEGER DEFAULT 0";
				cmd = new OleDbCommand(text3, con);
				cmd.ExecuteNonQuery();
				text3 = "Update Groups Set SyncEnabled=1 Where (Code1st In (-1,1,2,3,6,8,9,11,12,13,14,15,16,17,21,25,26,27,29,30)) or (Code2nd In (1,3,6,8,9,11,12,13,14,15,16,17,21,25,26,27,29,30)) or (Code3rd In (1,3,6,8,9,11,12,13,14,15,16,17,21,25,26,27,29,30)) or (Code4th In (1,3,6,8,9,11,12,13,14,15,16,17,21,25,26,27,29,30))";
				cmd = new OleDbCommand(text3, con);
				cmd.ExecuteNonQuery();
				text3 = "Update Groups Set SyncEnabled=0 Where (SyncEnabled<>1 or Isnull(SyncEnabled)=True)";
				cmd = new OleDbCommand(text3, con);
				cmd.ExecuteNonQuery();
			}
			text = "Ledgers";
			text2 = "SyncEnabled";
			field_exists = false;
			field_exists_or_not(text2, text);
			if (!field_exists)
			{
				string text4 = "ALTER TABLE " + text + " ADD COLUMN " + text2 + " INTEGER DEFAULT 0";
				cmd = new OleDbCommand(text4, con);
				cmd.ExecuteNonQuery();
				text4 = "Update Ledgers as A,Groups as B Set A.SyncEnabled=1 Where A.GroupCode=B.Code1st and B.SyncEnabled=1";
				cmd = new OleDbCommand(text4, con);
				cmd.ExecuteNonQuery();
				text4 = "Update Ledgers as A,Groups as B Set A.SyncEnabled=0 Where A.GroupCode=B.Code1st and B.SyncEnabled=0";
				cmd = new OleDbCommand(text4, con);
				cmd.ExecuteNonQuery();
			}
			text = "CompanyInfo";
			text2 = "SyncEnabled";
			field_exists = false;
			field_exists_or_not(text2, text);
			if (!field_exists)
			{
				string text5 = "ALTER TABLE " + text + " ADD COLUMN " + text2 + " INTEGER";
				cmd = new OleDbCommand(text5, con);
				cmd.ExecuteNonQuery();
				text5 = "Update CompanyInfo Set SyncEnabled=1";
				cmd = new OleDbCommand(text5, con);
				cmd.ExecuteNonQuery();
			}
		}

		private void field_exists_or_not(string field_nm, string tbl_nm)
		{
			//IL_001e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0028: Expected Obj, but got Unknown
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
									goto end_IL_0001;
								}
							}
							break;
						}
						case 260:
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
							Interaction.MsgBox(Information.Err().Description + "\r\n\r\n" + con.ConnectionString);
						}
						break;
						end_IL_0001_2:;
					}
					catch (Exception ex) when ((num2 != 0) & (num == 0))
					{
						ProjectData.SetProjectError(ex);
						try0001_dispatch = 260;
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

		private void btn_Fetch_data_Click(object sender, EventArgs e)
		{
			//IL_0033: Unknown result type (might be due to invalid IL or missing references)
			//IL_003d: Expected Obj, but got Unknown
			//IL_00b3: Unknown result type (might be due to invalid IL or missing references)
			//IL_00bd: Expected Obj, but got Unknown
			//IL_0133: Unknown result type (might be due to invalid IL or missing references)
			//IL_013d: Expected Obj, but got Unknown
			//IL_01b3: Unknown result type (might be due to invalid IL or missing references)
			//IL_01bd: Expected Obj, but got Unknown
			conn(Module1.db_path + "\\data\\data.031");
			btn_add_fields_Click(RuntimeHelpers.GetObjectValue(sender), e);
			string text = "Select CompanyName,FirmShortName,AccYearFrom,AccYearTo from CompanyInfo";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			if (dt.Rows.Count > 0)
			{
				DataGridView1.DataSource = dt;
			}
			Create_json_file_CompanyInfo();
			text = "Select GroupName,Code1st,Code2nd,Code3rd,Code4th from Groups";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			if (dt.Rows.Count > 0)
			{
				DataGridView1.DataSource = dt;
			}
			Create_json_file_Groups();
			text = "Select LedgerName,LedgerAlais,Code1st,GroupCode,InventoryChoice,MobNoForSMS,PartyStation,OpeningBal,OpeningType,CurrentBalance from Ledgers";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			if (dt.Rows.Count > 0)
			{
				DataGridView1.DataSource = dt;
			}
			Create_json_file_Ledgers();
			text = "Select RowNo,VoucherNumber,VoucherDate,TransType,AccountCode,DrCr,Amount,Narration,BalAmount from Transactions";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			if (dt.Rows.Count > 0)
			{
				DataGridView1.DataSource = dt;
			}
			Create_json_file_Transactions();
			string s = "base64 encoded string";
			byte[] bytes = Encoding.ASCII.GetBytes(s);
			string text2 = Convert.ToBase64String(bytes);
			string text3 = Module1.db_path + "\\" + file1 + ".txt";
			FileStream fileStream = File.Create(text3);
			byte[] bytes2 = new UTF8Encoding(encoderShouldEmitUTF8Identifier: true).GetBytes(combined_json);
			fileStream.Write(bytes2, 0, bytes2.Length);
			fileStream.Close();
			string source = text3;
			string destination = Module1.db_path + "\\" + file1 + ".zip";
			CreateZipFile(source, destination);
			process_finish = true;
		}

		private void Insert_Fields(string tbl_nm, string field_nm, string field_type, string val)
		{
			//IL_0059: Unknown result type (might be due to invalid IL or missing references)
			//IL_0063: Expected Obj, but got Unknown
			//IL_00b0: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ba: Expected Obj, but got Unknown
			field_exists = false;
			field_exists_or_not(field_nm, tbl_nm);
			if (!field_exists)
			{
				string text = "ALTER TABLE " + tbl_nm + " ADD COLUMN " + field_nm + " " + field_type;
				cmd = new OleDbCommand(text, con);
				cmd.ExecuteNonQuery();
				text = "Update " + tbl_nm + " set " + field_nm + "='" + val + "'";
				cmd = new OleDbCommand(text, con);
				cmd.ExecuteNonQuery();
			}
		}

		private void fetch_all_data()
		{
			//IL_0016: Unknown result type (might be due to invalid IL or missing references)
			//IL_0020: Expected Obj, but got Unknown
			//IL_00b4: Unknown result type (might be due to invalid IL or missing references)
			//IL_00be: Expected Obj, but got Unknown
			//IL_01a3: Unknown result type (might be due to invalid IL or missing references)
			//IL_01ad: Expected Obj, but got Unknown
			//IL_0292: Unknown result type (might be due to invalid IL or missing references)
			//IL_029c: Expected Obj, but got Unknown
			Add_fields();
			string text = "Select CompanyName,FirmShortName,AccYearFrom,AccYearTo,GSTIN from CompanyInfo";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			DataGridView2.DataSource = null;
			DataGridView2.Rows.Clear();
			if (dt.Rows.Count > 0)
			{
				DataGridView2.DataSource = dt;
			}
			Create_json_file_CompanyInfo();
			text = "Select GroupName,Code1st,Code2nd,Code3rd,Code4th,SyncEnabled from Groups";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			DataGridView2.DataSource = null;
			DataGridView2.Rows.Clear();
			if (dt.Rows.Count > 0)
			{
				DataGridView2.DataSource = dt;
			}
			Create_json_file_Groups();
			Insert_Fields("Ledgers", "MobNoForSMS", "String", "");
			Insert_Fields("Ledgers", "PartyStation", "String", "");
			Insert_Fields("Ledgers", "GSTIN", "String", "");
			text = "Select LedgerName,LedgerAlais,Code1st,GroupCode,InventoryChoice,MobNoForSMS,PartyStation,OpeningBal,OpeningType,CurrentBalance,SyncEnabled,MailingName,GSTIN from Ledgers";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			DataGridView2.DataSource = null;
			DataGridView2.Rows.Clear();
			if (dt.Rows.Count > 0)
			{
				DataGridView2.DataSource = dt;
			}
			Create_json_file_Ledgers();
			Insert_Fields("Transactions", "DebitCreditNote", "Integer", "0");
			Insert_Fields("Transactions", "BrokerName", "String", "");
			Insert_Fields("Transactions", "BalAmount", "Double", "0");
			text = "Select RowNo,VoucherNumber,VoucherDate,TransType,AccountCode,DrCr,Amount,Narration,BalAmount from Transactions";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			DataGridView2.DataSource = null;
			DataGridView2.Rows.Clear();
			if (dt.Rows.Count > 0)
			{
				DataGridView2.DataSource = dt;
			}
			Create_json_file_Transactions();
			string s = combined_json;
			byte[] bytes = Encoding.ASCII.GetBytes(s);
			string s2 = Convert.ToBase64String(bytes);
			string text2 = Module1.db_path + "\\" + file1 + ".txt";
			FileStream fileStream = File.Create(text2);
			byte[] bytes2 = new UTF8Encoding(encoderShouldEmitUTF8Identifier: true).GetBytes(s2);
			fileStream.Write(bytes2, 0, bytes2.Length);
			fileStream.Close();
			string text3 = text2;
			string text4 = Module1.db_path + "\\" + file1 + ".zip";
			CreateZipFile(text3, text4);
			GET_FTP_Crediential();
			UploadFileNEW(Module1.db_path + "\\\\" + file1 + ".zip", ftp_remote_host + "/" + file1 + ".zip", ftp_username, ftp_password);
			Thread.Sleep(1000);
			int try040b_dispatch = -1;
			while (true)
			{
				try
				{
					/*Note: ILSpy has introduced the following switch to emulate a goto from catch-block to try-block*/;
					switch (try040b_dispatch)
					{
					default:
						((ServerComputer)MyProject.Computer).FileSystem.DeleteFile(text3);
						((ServerComputer)MyProject.Computer).FileSystem.DeleteFile(text4);
						break;
					}
				}
				catch (Exception ex)
				{
					ProjectData.SetProjectError(ex);
					Exception ex2 = ex;
					ProjectData.ClearProjectError();
					try040b_dispatch = 1036;
					continue;
				}
				break;
			}
			process_finish = true;
		}

		private void Create_json_file_Extra()
		{
			checked
			{
				int num = DataGridView1.RowCount - 1;
				string text5 = default;
				string text4 = default;
				for (int i = 0; i <= num && !Operators.ConditionalCompareObjectEqual(DataGridView1.Rows[i].Cells[0].Value, null, TextCompare: false); i++)
				{
					string text = "\"id\":\"" + Conversions.ToString(i + 1) + "\"";
					string text2 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"AccountingYearFrom\":\"", DataGridView1.Rows[i].Cells[0].Value), "\""));
					string text3 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"AccountingYearTo\":\"", DataGridView1.Rows[i].Cells[1].Value), "\""));
					text4 = ((i != 0) ? (text4 + ",{" + text + "," + text2 + "," + text3 + "," + text5 + "}") : ("{" + text + "," + text2 + "," + text3 + "," + text5 + "}"));
				}
				text4 = "[" + text4 + "]";
				combined_json = combined_json + "\"TempLastEnteredVoucher\":" + text4 + ",";
			}
		}

		private void Create_json_file_CompanyInfo()
		{
			//IL_01d4: Unknown result type (might be due to invalid IL or missing references)
			//IL_01de: Expected Obj, but got Unknown
			checked
			{
				int num = DataGridView2.RowCount - 1;
				string text7 = default;
				string text8 = default;
				for (int i = 0; i <= num && !Operators.ConditionalCompareObjectEqual(DataGridView2.Rows[i].Cells[0].Value, null, TextCompare: false); i++)
				{
					string text = "\"id\":\"" + Conversions.ToString(i + 1) + "\"";
					string text2 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"CompanyName\":\"", DataGridView2.Rows[i].Cells[0].Value), "\""));
					string text3 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"FirmShortName\":\"", DataGridView2.Rows[i].Cells[1].Value), "\""));
					string text4 = "\"CompanyYear\":\"" + Strings.Right(Conversions.ToString(DataGridView2.Rows[i].Cells[2].Value), 4) + "-" + Strings.Right(Conversions.ToString(DataGridView2.Rows[i].Cells[3].Value), 4) + "\"";
					string text5 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"GSTIN\":\"", DataGridView2.Rows[i].Cells[4].Value), "\""));
					GSTIN = DataGridView2.Rows[i].Cells[4].Value.ToString();
					string text6 = "SELECT MAX(VoucherDate) AS Expr1 FROM Transactions";
					da = new OleDbDataAdapter(text6, con);
					ds = new DataSet();
					((DbDataAdapter)(object)da).Fill(ds);
					dt = ds.Tables[0];
					if (dt.Rows.Count > 0)
					{
						text7 = dt.Rows[0][0].ToString();
						text7 = "\"MaxVoucherDate\":\"" + text7 + "\"";
					}
					text8 = ((i != 0) ? (text8 + ",{" + text + "," + text2 + "," + text3 + "," + text4 + "," + text7 + "," + text5 + "}") : ("{" + text + "," + text2 + "," + text3 + "," + text4 + "," + text7 + "," + text5 + "}"));
				}
				text8 = "[" + text8 + "]";
				combined_json = "{\"Company\":" + text8 + ",";
			}
		}

		private void Create_json_file_Groups()
		{
			checked
			{
				int num = DataGridView2.RowCount - 1;
				string text8 = default;
				for (int i = 0; i <= num && !Operators.ConditionalCompareObjectEqual(DataGridView2.Rows[i].Cells[0].Value, null, TextCompare: false); i++)
				{
					string text = "\"id\":\"" + Conversions.ToString(i + 1) + "\"";
					string text2 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"GroupName\":\"", DataGridView2.Rows[i].Cells[0].Value), "\""));
					string text3 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"Code1st\":\"", DataGridView2.Rows[i].Cells[1].Value), "\""));
					string text4 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"Code2nd\":\"", DataGridView2.Rows[i].Cells[2].Value), "\""));
					string text5 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"Code3rd\":\"", DataGridView2.Rows[i].Cells[3].Value), "\""));
					string text6 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"Code4th\":\"", DataGridView2.Rows[i].Cells[4].Value), "\""));
					string text7 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"SyncEnabled\":\"", DataGridView2.Rows[i].Cells[5].Value), "\""));
					text8 = ((i != 0) ? (text8 + ",{" + text + "," + text2 + "," + text3 + "," + text4 + "," + text5 + "," + text6 + "," + text7 + "}") : ("{" + text + "," + text2 + "," + text3 + "," + text4 + "," + text5 + "," + text6 + "," + text7 + "}"));
				}
				text8 = "[" + text8 + "]";
				combined_json = combined_json + "\"Group\":" + text8 + ",";
			}
		}

		private void Create_json_file_Ledgers()
		{
			checked
			{
				int num = DataGridView2.RowCount - 1;
				string text16 = default;
				for (int i = 0; i <= num && !Operators.ConditionalCompareObjectEqual(DataGridView2.Rows[i].Cells[0].Value, null, TextCompare: false); i++)
				{
					string text = "";
					string text2 = "\"id\":\"" + Conversions.ToString(i + 1) + "\"";
					string text3 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"LedgerName\":\"", DataGridView2.Rows[i].Cells[0].Value), "\""));
					string text4 = "\"LedgerAlais\":\"" + text + "\"";
					string text5 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"Code1st\":\"", DataGridView2.Rows[i].Cells[2].Value), "\""));
					string text6 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"GroupCode\":\"", DataGridView2.Rows[i].Cells[3].Value), "\""));
					string text7 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"InventoryChoice\":\"", DataGridView2.Rows[i].Cells[4].Value), "\""));
					string text8 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"MobNoForSMS\":\"", DataGridView2.Rows[i].Cells[5].Value), "\""));
					string text9 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"PartyStation\":\"", DataGridView2.Rows[i].Cells[6].Value), "\""));
					string text10 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"OpeningBal\":\"", DataGridView2.Rows[i].Cells[7].Value), "\""));
					string text11 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"OpeningType\":\"", DataGridView2.Rows[i].Cells[8].Value), "\""));
					string text12 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"CurrentBalance\":\"", DataGridView2.Rows[i].Cells[9].Value), "\""));
					string text13 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"SyncEnabled\":\"", DataGridView2.Rows[i].Cells[10].Value), "\""));
					string text14 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"MailingName\":\"", DataGridView2.Rows[i].Cells[11].Value), "\""));
					string text15 = Conversions.ToString(Operators.ConcatenateObject(Operators.ConcatenateObject("\"GSTIN\":\"", DataGridView2.Rows[i].Cells[12].Value), "\""));
					text16 = ((i != 0) ? (text16 + ",{" + text2 + "," + text3 + "," + text4 + "," + text5 + "," + text6 + "," + text7 + "," + text8 + "," + text9 + "," + text10 + "," + text11 + "," + text12 + "," + text13 + "," + text14 + "," + text15 + "}") : ("{" + text2 + "," + text3 + "," + text4 + "," + text5 + "," + text6 + "," + text7 + "," + text8 + "," + text9 + "," + text10 + "," + text11 + "," + text12 + "," + text13 + "," + text14 + "," + text15 + "}"));
				}
				text16 = "[" + text16 + "]";
				text16 = text16.Replace("\\", "/");
				combined_json = combined_json + "\"Ledger\":" + text16 + ",";
			}
		}

		private void Create_json_file_Transactions()
		{
			//IL_002a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0034: Expected Obj, but got Unknown
			//IL_00b4: Unknown result type (might be due to invalid IL or missing references)
			//IL_00be: Expected Obj, but got Unknown
			//IL_0125: Unknown result type (might be due to invalid IL or missing references)
			//IL_012f: Expected Obj, but got Unknown
			//IL_2033: Unknown result type (might be due to invalid IL or missing references)
			//IL_203d: Expected Obj, but got Unknown
			//IL_1f63: Unknown result type (might be due to invalid IL or missing references)
			//IL_1f6d: Expected Obj, but got Unknown
			Insert_Fields("VoucherSettings", "ShowItemNarrationInLedger", "Int", "0");
			string text = "SELECT ShowItemNarrationInLedger from VoucherSettings";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			int num = default;
			if (dt.Rows.Count > 0)
			{
				num = Conversions.ToInteger(dt.Rows[0].ItemArray[0].ToString());
			}
			text = "SELECT Weight from StockTransactions Where Weight<>0";
			da = new OleDbDataAdapter(text, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			int num2 = 0;
			if (dt.Rows.Count > 0)
			{
				num2 = 1;
			}
			string text2 = "SELECT StockTransactions.VoucherNumber,StockTransactions.TransType,StockTransactions.ItemCode, StockTransactions.Bags, StockTransactions.Weight, StockTransactions.Rate, StockTransactions.Amount, StockTransactions.Narration, StockTransactions.DheriBillNo, StockTransactions.Packing, StockTransactions.TransType,StockItems.Code1st, StockItems.ItemName, StockUnits.UnitName FROM ((StockTransactions INNER JOIN StockItems  ON StockTransactions.ItemCode = StockItems.Code1st) INNER JOIN StockUnits ON StockItems.UnitCode = StockUnits.Code1st)WHERE (StockTransactions.TransType = 'Sale' OR StockTransactions.TransType = 'Purc' OR StockTransactions.TransType = 'JFrm' OR StockTransactions.TransType = 'IFrm' OR StockTransactions.TransType = 'SlRn' OR StockTransactions.TransType = 'PrRn'  OR StockTransactions.TransType = 'IFrm' OR StockTransactions.TransType = 'PrRn' OR StockTransactions.TransType = 'Slrn')";
			text2 = "SELECT StockTransactions.VoucherNumber, StockTransactions.TransType, StockTransactions.ItemCode, StockTransactions.Bags, StockTransactions.Weight, StockTransactions.Rate, StockTransactions.Amount,StockTransactions.Narration, StockTransactions.DheriBillNo, StockTransactions.Packing, StockTransactions.TransType AS Type, StockItems.Code1st, StockItems.ItemName, StockUnits.UnitName,  Transactions.InvoiceNo, Transactions.PurchaseType, Transactions.EntryType, Transactions.Narration AS TransNarration, Transactions.Amount AS TransAmount, Transactions.DrCr,Transactions.VoucherDate FROM (((StockTransactions INNER JOIN  StockItems ON StockTransactions.ItemCode = StockItems.Code1st) INNER JOIN StockUnits ON StockItems.UnitCode = StockUnits.Code1st) INNER JOIN Transactions ON StockTransactions.VoucherNumber = Transactions.VoucherNumber  AND StockTransactions.TransType = Transactions.TransType AND StockTransactions.VoucherDate = Transactions.VoucherDate) WHERE    Transactions.RowNo=1 and ((StockTransactions.TransType = 'Sale') OR (StockTransactions.TransType = 'Purc') OR (StockTransactions.TransType = 'JFrm') OR  (StockTransactions.TransType = 'IFrm') OR (StockTransactions.TransType = 'SlRn') OR  (StockTransactions.TransType = 'PrRn') OR (StockTransactions.TransType = 'IFrm') OR  (StockTransactions.TransType = 'PrRn') OR (StockTransactions.TransType = 'Slrn'))  Order By StockTransactions.VoucherDate,StockTransactions.TransType,StockTransactions.VoucherNumber,StockTransactions.rowno";
			da = new OleDbDataAdapter(text2, con);
			ds = new DataSet();
			((DbDataAdapter)(object)da).Fill(ds);
			dt = ds.Tables[0];
			DataGridView1.Rows.Clear();
			DataGridView1.ColumnCount = 4;
			checked
			{
				if (dt.Rows.Count > 0)
				{
					Application.DoEvents();
					((Control)pb1).Visible = true;
					pb1.Maximum = dt.Rows.Count + 2;
					Label4.Text = "Stage1 Processing...";
					string text3 = "";
					if ((Conversion.Val(num2) == 0.0) & (Conversion.Val(num) == 0.0))
					{
						int num3 = dt.Rows.Count - 1;
						for (int i = 0; i <= num3; i++)
						{
							text3 = "";
							string text4 = "";
							string text5 = "";
							string text6 = "";
							text6 = dt.Rows[i][14].ToString();
							text4 = dt.Rows[i][1].ToString();
							if ((Operators.CompareString(text4, "Purc", TextCompare: false) == 0) | (Operators.CompareString(text4, "Sale", TextCompare: false) == 0))
							{
								text5 = "Invoice No. " + text6;
							}
							else if ((Operators.CompareString(text4, "PrRn", TextCompare: false) == 0) | (Operators.CompareString(text4, "SlRn", TextCompare: false) == 0))
							{
								text5 = "Goods Return (" + text6 + ")";
							}
							else if ((Operators.CompareString(text4, "Jrnl", TextCompare: false) == 0) & (Operators.CompareString(dt.Rows[i][15].ToString(), "DebitCreditNote", TextCompare: false) == 0))
							{
								text5 = "Note No. " + text6;
							}
							int num4 = dt.Rows.Count - 1;
							for (int j = 0; j <= num4; j++)
							{
								if (!((Operators.CompareString(dt.Rows[i][0].ToString(), dt.Rows[j][0].ToString(), TextCompare: false) == 0) & (Operators.CompareString(dt.Rows[i][1].ToString(), dt.Rows[j][1].ToString(), TextCompare: false) == 0) & (Operators.CompareString(dt.Rows[i][20].ToString(), dt.Rows[j][20].ToString(), TextCompare: false) == 0)))
								{
									continue;
								}
								if ((Operators.CompareString(dt.Rows[j][10].ToString(), "Purc", TextCompare: false) == 0) | (Operators.CompareString(dt.Rows[j][10].ToString(), "Sale", TextCompare: false) == 0) | (Operators.CompareString(dt.Rows[j][10].ToString(), "SlRn", TextCompare: false) == 0) | (Operators.CompareString(dt.Rows[j][10].ToString(), "PrRn", TextCompare: false) == 0))
								{
									if (Operators.CompareString(text3, "", TextCompare: false) == 0)
									{
										if (Operators.CompareString(dt.Rows[j][13].ToString(), "None", TextCompare: false) == 0)
										{
											text3 = dt.Rows[j][12].ToString() + "  = " + Strings.Format(Conversion.Val(dt.Rows[j][6].ToString()), "0.00");
											text3 = text3.Replace("None", "");
										}
										else
										{
											text3 = dt.Rows[j][12].ToString() + " " + dt.Rows[j][3].ToString() + " " + dt.Rows[j][13].ToString() + " @ " + Strings.Format(Conversion.Val(dt.Rows[j][5].ToString()), "0.00");
										}
									}
									else if (Operators.CompareString(dt.Rows[j][13].ToString(), "None", TextCompare: false) == 0)
									{
										text3 = text3 + " " + dt.Rows[j][12].ToString() + " = " + Strings.Format(Conversion.Val(dt.Rows[j][6].ToString()), "0.00");
									}
									else
									{
										text3 = text3 + " " + dt.Rows[j][12].ToString() + " " + dt.Rows[j][3].ToString() + " " + dt.Rows[j][13].ToString() + " @ " + Strings.Format(Conversion.Val(dt.Rows[j][5].ToString()), "0.00");
										text3 = text3.Replace("None", "");
									}
									Console.WriteLine(dt.Rows[j][12].ToString() + "       ::::::       " + text3);
								}
								text3 += "|";
							}
							text3 = text5 + "|" + text3 + "|" + dt.Rows[i][17].ToString();
							text3 = text3.Replace("||", "|");
							DataGridView1.RowCount += 1;
							DataGridView1.Rows[i].Cells[0].Value = dt.Rows[i][0].ToString();
							DataGridView1.Rows[i].Cells[1].Value = dt.Rows[i][1].ToString();
							DataGridView1.Rows[i].Cells[2].Value = text3;
							DataGridView1.Rows[i].Cells[3].Value = dt.Rows[i][20].ToString();
							Application.DoEvents();
							pb1.Value = i + 1;
							Label4.Text = "Stage1  " + Conversions.ToString(Math.Ceiling(Conversion.Val(i) * 100.0 / Conversion.Val(dt.Rows.Count))) + "% Completed";
							Thread.Sleep(1);
						}
					}
					else
					{
						int num5 = dt.Rows.Count - 1;
						for (int k = 0; k <= num5; k++)
						{
							text3 = "";
							string text4 = "";
							string text5 = "";
							string text6 = "";
							text6 = dt.Rows[k][14].ToString();
							text4 = dt.Rows[k][1].ToString();
							if ((Operators.CompareString(text4, "Purc", TextCompare: false) == 0) | (Operators.CompareString(text4, "Sale", TextCompare: false) == 0))
							{
								text5 = "Invoice No. " + text6;
							}
							else if ((Operators.CompareString(text4, "PrRn", TextCompare: false) == 0) | (Operators.CompareString(text4, "SlRn", TextCompare: false) == 0))
							{
								text5 = "Goods Return (" + text6 + ")";
							}
							else if (Operators.CompareString(text4, "JFrm", TextCompare: false) == 0)
							{
								text5 = ((Operators.CompareString(firm_state, "RAJASTHAN", TextCompare: false) != 0) ? ("J.Form No. " + text6) : ("Parchi No. " + text6));
							}
							else if (Operators.CompareString(text4, "IFrm", TextCompare: false) == 0)
							{
								text5 = ((Operators.CompareString(firm_state, "RAJASTHAN", TextCompare: false) != 0) ? ("I.Form No. " + text6) : ("Bill No. " + text6));
							}
							else if ((Operators.CompareString(text4, "Jrnl", TextCompare: false) == 0) & (Operators.CompareString(dt.Rows[k][15].ToString(), "DebitCreditNote", TextCompare: false) == 0))
							{
								text5 = "Note No. " + text6;
							}
							int num6 = dt.Rows.Count - 1;
							for (int l = 0; l <= num6; l++)
							{
								if (!((Operators.CompareString(dt.Rows[k][0].ToString(), dt.Rows[l][0].ToString(), TextCompare: false) == 0) & (Operators.CompareString(dt.Rows[k][1].ToString(), dt.Rows[l][1].ToString(), TextCompare: false) == 0) & (Operators.CompareString(dt.Rows[k][20].ToString(), dt.Rows[l][20].ToString(), TextCompare: false) == 0)))
								{
									continue;
								}
								if ((Operators.CompareString(dt.Rows[l][10].ToString(), "Purc", TextCompare: false) == 0) | (Operators.CompareString(dt.Rows[l][10].ToString(), "Sale", TextCompare: false) == 0) | (Operators.CompareString(dt.Rows[l][10].ToString(), "JFrm", TextCompare: false) == 0) | (Operators.CompareString(dt.Rows[l][10].ToString(), "IFrm", TextCompare: false) == 0) | (Operators.CompareString(dt.Rows[l][10].ToString(), "SlRn", TextCompare: false) == 0) | (Operators.CompareString(dt.Rows[l][10].ToString(), "PrRn", TextCompare: false) == 0))
								{
									if (Operators.CompareString(dt.Rows[l][13].ToString(), "Qtl.", TextCompare: false) == 0)
									{
										if (Conversion.Val(dt.Rows[l][3].ToString()) > 0.0)
										{
											text3 = ((Operators.CompareString(text3, "", TextCompare: false) != 0) ? (text3 + " " + dt.Rows[l][12].ToString() + " " + dt.Rows[l][3].ToString() + "# " + Strings.Format(Conversion.Val(dt.Rows[l][4].ToString()), "0.000") + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")) : (dt.Rows[l][12].ToString() + " " + dt.Rows[l][3].ToString() + "# " + Strings.Format(Conversion.Val(dt.Rows[l][4].ToString()), "0.000") + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")));
										}
										else
										{
											text3 = ((Operators.CompareString(text3, "", TextCompare: false) != 0) ? (text3 + " " + dt.Rows[l][12].ToString() + " " + Strings.Format(Conversion.Val(dt.Rows[l][4].ToString()), "0.000") + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")) : (dt.Rows[l][12].ToString() + " " + Strings.Format(Conversion.Val(dt.Rows[l][4].ToString()), "0.000") + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")));
										}
									}
									else
									{
										if (num != 1)
										{
											text3 = ((Operators.CompareString(text3, "", TextCompare: false) != 0) ? (text3 + " " + dt.Rows[l][12].ToString() + " " + dt.Rows[l][3].ToString() + " " + dt.Rows[l][13].ToString() + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")) : (dt.Rows[l][12].ToString() + " " + dt.Rows[l][3].ToString() + " " + dt.Rows[l][13].ToString() + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")));
										}
										else
										{
											if (Strings.Len(dt.Rows[l][8].ToString()) > 0)
											{
												text3 = ((Operators.CompareString(text3, "", TextCompare: false) != 0) ? (text3 + " " + dt.Rows[l][12].ToString() + " " + dt.Rows[l][8].ToString() + " " + dt.Rows[l][3].ToString() + " " + dt.Rows[l][13].ToString() + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")) : (dt.Rows[l][12].ToString() + " " + dt.Rows[l][8].ToString() + " " + dt.Rows[l][3].ToString() + " " + dt.Rows[l][13].ToString() + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")));
											}
											else if (Strings.Len(dt.Rows[l][7].ToString()) > 0)
											{
												text3 = ((Operators.CompareString(text3, "", TextCompare: false) != 0) ? (text3 + " " + dt.Rows[l][12].ToString() + " " + dt.Rows[l][7].ToString() + " " + dt.Rows[l][3].ToString() + " " + dt.Rows[l][13].ToString() + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")) : (dt.Rows[l][12].ToString() + " " + dt.Rows[l][7].ToString() + " " + dt.Rows[l][3].ToString() + " " + dt.Rows[l][13].ToString() + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")));
											}
											else
											{
												text3 = ((Operators.CompareString(text3, "", TextCompare: false) != 0) ? (text3 + " " + dt.Rows[l][12].ToString() + " " + dt.Rows[l][3].ToString() + " " + dt.Rows[l][13].ToString() + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")) : (dt.Rows[l][12].ToString() + " " + dt.Rows[l][3].ToString() + " " + dt.Rows[l][13].ToString() + " @ " + Strings.Format(Conversion.Val(dt.Rows[l][5].ToString()), "0.00")));
											}
											Console.WriteLine(dt.Rows[l][12].ToString() + "       ::::::       " + text3);
										}
										if (Operators.CompareString(dt.Rows[l][13].ToString(), "None", TextCompare: false) == 0)
										{
											text3 = text3.Replace("0 None @ 0.00", "= " + Strings.Format(Conversion.Val(dt.Rows[l][6].ToString()), "0.00"));
										}
									}
								}
								text3 += "|";
							}
							text3 = text5 + "|" + text3 + "|" + dt.Rows[k][17].ToString();
							text3 = text3.Replace("||", "|");
							DataGridView1.RowCount += 1;
							DataGridView1.Rows[k].Cells[0].Value = dt.Rows[k][0].ToString();
							DataGridView1.Rows[k].Cells[1].Value = dt.Rows[k][1].ToString();
							DataGridView1.Rows[k].Cells[2].Value = text3;
							DataGridView1.Rows[k].Cells[3].Value = dt.Rows[k][20].ToString();
							Application.DoEvents();
							pb1.Value = k + 1;
							Label4.Text = "Stage1  " + Conversions.ToString(Math.Ceiling(Conversion.Val(k) * 100.0 / Conversion.Val(dt.Rows.Count))) + "% Completed";
							Thread.Sleep(1);
						}
					}
					Label4.Text = "Stage1  100% Completed";
				}
				int num7 = DataGridView1.Rows.Count - 1;
				for (int m = num7; m >= 0; m += -1)
				{
					int num8 = m - 1;
					for (int n = num8; n >= 0; n += -1)
					{
						if (Conversions.ToBoolean(Operators.AndObject(Operators.AndObject(Operators.CompareObjectEqual(DataGridView1.Rows[m].Cells[0].Value, DataGridView1.Rows[n].Cells[0].Value, TextCompare: false), Operators.CompareObjectEqual(DataGridView1.Rows[m].Cells[1].Value, DataGridView1.Rows[n].Cells[1].Value, TextCompare: false)), Operators.CompareObjectEqual(DataGridView1.Rows[m].Cells[3].Value, DataGridView1.Rows[n].Cells[3].Value, TextCompare: false))))
						{
							DataGridView1.Rows.RemoveAt(m);
							break;
						}
					}
				}
				Application.DoEvents();
				Create_Table();
				pb1.Value = 0;
				pb1.Maximum = DataGridView1.RowCount + 2;
				Label4.Text = "Stage2 Processing...";
				int num9 = DataGridView1.RowCount - 1;
				for (int num10 = 0; num10 <= num9; num10++)
				{
					long num11 = Conversions.ToLong(DataGridView1.Rows[num10].Cells[0].Value);
					string text7 = Conversions.ToString(DataGridView1.Rows[num10].Cells[1].Value);
					string text8 = Conversions.ToString(DataGridView1.Rows[num10].Cells[2].Value);
					string text9 = Conversions.ToString(DataGridView1.Rows[num10].Cells[3].Value);
					if ((Operators.CompareString(text9, null, TextCompare: false) == 0) | (Operators.CompareString(text9, "", TextCompare: false) == 0))
					{
						break;
					}
					string text10 = "insert into NewTable(VoucherNo,VoucherType, details,vdate) values (" + Conversions.ToString(num11) + ",'" + text7 + "','" + text8 + "',#" + Strings.Format(Conversions.ToDate(text9), "dd-MMM-yyyy") + "#)";
					cmd = new OleDbCommand(text10, con);
					cmd.ExecuteNonQuery();
					Application.DoEvents();
					pb1.Value = num10 + 1;
					Label4.Text = "Stage2  " + Conversions.ToString(Math.Ceiling(Conversion.Val(num10) * 100.0 / Conversion.Val(DataGridView1.RowCount))) + "% Completed";
					Thread.Sleep(1);
				}
				Label4.Text = "Stage2  100% Completed";
				text2 = "Select RowNo,VoucherNumber,Format([VoucherDate],'dd-mm-yyyy') as VoucherDate,TransType,AccountCode,DrCr,Amount,Narration,BalAmount,InvoiceNo from Transactions";
				text2 = "SELECT Transactions.RowNo, Transactions.VoucherNumber, Format([Transactions.VoucherDate],'dd-mm-yyyy') as VoucherDate, Transactions.TransType, Transactions.AccountCode, Transactions.DrCr, Transactions.Amount, Transactions.Narration, Transactions.BalAmount, Transactions.InvoiceNo, NewTable.details FROM Transactions INNER JOIN NewTable ON Transactions.VoucherNumber = NewTable.VoucherNo";
				text2 = "SELECT Transactions.RowNo, Transactions.VoucherNumber, Format([Transactions.VoucherDate], 'dd-mm-yyyy') AS VoucherDate, Transactions.TransType, Transactions.AccountCode, Transactions.DrCr,  Transactions.Amount, Transactions.Narration, Transactions.BalAmount, Transactions.InvoiceNo, NewTable.details FROM Transactions LEFT OUTER JOIN NewTable ON Transactions.TransType = NewTable.VoucherType AND Transactions.VoucherNumber = NewTable.VoucherNo";
				text2 = "SELECT Transactions.RowNo, Transactions.VoucherNumber, Format([Transactions.VoucherDate], 'dd-mm-yyyy') AS VoucherDate, Transactions.TransType, Transactions.AccountCode, Transactions.DrCr, Transactions.Amount, Transactions.Narration, Transactions.BalAmount, Transactions.InvoiceNo, NewTable.details FROM  ((Transactions INNER JOIN Ledgers ON Transactions.AccountCode = Ledgers.Code1st)  LEFT OUTER JOIN NewTable ON Transactions.TransType = NewTable.VoucherType AND Transactions.VoucherNumber = NewTable.VoucherNo) WHERE (Ledgers.SyncEnabled = 1 or Ledgers.SyncEnabled = True)";
				text2 = "SELECT Transactions.RowNo, Transactions.VoucherNumber, Format([Transactions.VoucherDate], 'dd-mm-yyyy') AS VoucherDate, Transactions.TransType, Transactions.AccountCode, Transactions.DrCr, Transactions.Amount, Transactions.Narration, Transactions.BalAmount, Transactions.InvoiceNo, NewTable.details FROM  ((Transactions INNER JOIN Ledgers ON Transactions.AccountCode = Ledgers.Code1st)  LEFT OUTER JOIN NewTable ON Transactions.TransType = NewTable.VoucherType AND Transactions.VoucherNumber = NewTable.VoucherNo)";
				text2 = "SELECT Transactions.RowNo, Transactions.VoucherNumber, Format([Transactions.VoucherDate], 'dd-mm-yyyy') AS VoucherDate, Transactions.TransType, Transactions.AccountCode, Transactions.DrCr, Transactions.Amount, Transactions.Narration, Transactions.BalAmount, Transactions.InvoiceNo, NewTable.details FROM ((Transactions INNER JOIN Ledgers ON Transactions.AccountCode = Ledgers.Code1st) LEFT OUTER JOIN NewTable ON Transactions.VoucherDate  = NewTable.vdate AND Transactions.TransType = NewTable.VoucherType AND Transactions.VoucherNumber  = NewTable.VoucherNo)";
				da = new OleDbDataAdapter(text2, con);
				ds = new DataSet();
				((DbDataAdapter)(object)da).Fill(ds);
				dt = ds.Tables[0];
				string json = GetJson(dt);
				combined_json = combined_json + "\"Transaction\":" + json + "}";
			}
		}

		private void Button6_Click_1(object sender, EventArgs e)
		{
			DateTime lastWriteTime = File.GetLastWriteTime(Module1.db_path + "\\Data\\data.001");
			Interaction.MsgBox(lastWriteTime);
		}

		public void UploadFileNEW(string _FileName, string _UploadPath, string _FTPUser, string _FTPPass)
		{
			FileInfo fileInfo = new FileInfo(_FileName);
			FtpWebRequest ftpWebRequest = (FtpWebRequest)WebRequest.Create(new Uri(_UploadPath));
			ftpWebRequest.Credentials = new NetworkCredential(_FTPUser, _FTPPass);
			ftpWebRequest.KeepAlive = false;
			ftpWebRequest.Timeout = 20000;
			ftpWebRequest.Method = "STOR";
			ftpWebRequest.UseBinary = true;
			ftpWebRequest.ContentLength = fileInfo.Length;
			int num = 2048;
			byte[] buffer = new byte[checked(num - 1 + 1)];
			FileStream fileStream = fileInfo.OpenRead();
			try
			{
				Stream requestStream = ftpWebRequest.GetRequestStream();
				for (int num2 = fileStream.Read(buffer, 0, num); num2 != 0; num2 = fileStream.Read(buffer, 0, num))
				{
					requestStream.Write(buffer, 0, num2);
				}
				requestStream.Close();
				requestStream.Dispose();
				fileStream.Close();
				fileStream.Dispose();
			}
			catch (Exception ex)
			{
				ProjectData.SetProjectError(ex);
				Exception ex2 = ex;
				NameValueCollection nameValueCollection = new NameValueCollection();
				nameValueCollection.Add("images", _FileName);
				string left = UploadFilesToRemoteUrl("http://kamrasoftwares.com/bahikhata/fileupload.php", new string[1] { _FileName }, nameValueCollection);
				if (Operators.CompareString(left, "test", TextCompare: false) == 0)
				{
					Interaction.MsgBox("File Not Sent", MsgBoxStyle.Critical, "Kamra Softwares");
				}
				ProjectData.ClearProjectError();
			}
		}

		private void Form1_KeyDown(object sender, KeyEventArgs e)
		{
			//IL_0002: Unknown result type (might be due to invalid IL or missing references)
			//IL_0009: Invalid comparison between Unknown and I4
			//IL_0052: Unknown result type (might be due to invalid IL or missing references)
			//IL_0059: Invalid comparison between Unknown and I4
			//IL_00fa: Unknown result type (might be due to invalid IL or missing references)
			//IL_0101: Invalid comparison between Unknown and I4
			//IL_019c: Unknown result type (might be due to invalid IL or missing references)
			//IL_01a3: Invalid comparison between Unknown and I4
			//IL_026a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0271: Invalid comparison between Unknown and I4
			if ((int)e.KeyCode == 27)
			{
				string value = Conversions.ToString((int)Interaction.MsgBox("Do You Want to Exit ?", MsgBoxStyle.YesNo | MsgBoxStyle.Question, Module1.Title_msg));
				if (Conversions.ToDouble(value) == 6.0)
				{
					Application.Exit();
				}
				return;
			}
			if (e.Control && (int)e.KeyCode == 65)
			{
				checked
				{
					int num = grid3.Rows.Count - 1;
					for (int i = 0; i <= num && !Operators.ConditionalCompareObjectEqual(grid3.Rows[i].Cells[1].Value, "", TextCompare: false); i++)
					{
						grid3.Rows[i].Cells[0].Value = true;
					}
					Count_selected_cmps();
				}
			}
			if (e.Control && (int)e.KeyCode == 85)
			{
				checked
				{
					int num2 = grid3.Rows.Count - 1;
					for (int j = 0; j <= num2 && !Operators.ConditionalCompareObjectEqual(grid3.Rows[j].Cells[1].Value, "", TextCompare: false); j++)
					{
						grid3.Rows[j].Cells[0].Value = false;
					}
					Count_selected_cmps();
				}
			}
			if ((int)e.KeyCode == 116)
			{
				if (grid3.Columns[6].Visible)
				{
					grid3.Columns[6].Visible = false;
					grid3.Columns[1].HeaderText = "Firm Name  (Ctrl+A: Select All  -  Ctrl+U: Unselect All)             ";
				}
				else
				{
					grid3.Columns[6].Visible = true;
					grid3.Columns[1].HeaderText = "Firm Name  (Ctrl+A: Select All  -  Ctrl+U: Unselect All) ";
				}
				if (((Control)Lbl_UserID).Visible)
				{
					((Control)Lbl_UserID).Visible = false;
				}
				else
				{
					((Control)Lbl_UserID).Visible = true;
				}
			}
			if ((int)e.KeyCode == 117)
			{
				string value2 = Conversions.ToString((int)Interaction.MsgBox("Are you sure want to Reset CompanyID for " + grid3.SelectedRows[0].Cells[1].Value.ToString() + " ?", MsgBoxStyle.YesNo | MsgBoxStyle.Question, "Kamra Softwares"));
				if (Conversions.ToDouble(value2) == 6.0)
				{
					ResetCompannyID(grid3.SelectedRows[0].Cells[3].Value.ToString());
					grid3.SelectedRows[0].Cells[6].Value = 0;
				}
			}
		}

		private void Form1_Closing(object sender, CancelEventArgs e)
		{
			Application.Exit();
		}

		public string UploadFilesToRemoteUrl(string url, string[] files, NameValueCollection formFields = null)
		{
			string text = "----------------------------" + DateTime.Now.Ticks.ToString("x");
			HttpWebRequest httpWebRequest = (HttpWebRequest)WebRequest.Create(url);
			httpWebRequest.ContentType = "multipart/form-data; boundary=" + text;
			httpWebRequest.Method = "POST";
			httpWebRequest.KeepAlive = true;
			Stream stream = new MemoryStream();
			byte[] bytes = Encoding.ASCII.GetBytes("\r\n--" + text + "\r\n");
			byte[] bytes2 = Encoding.ASCII.GetBytes("\r\n--" + text + "--");
			string format = "\r\n--" + text + "\r\nContent-Disposition: form-data; name=\"{0}\";\r\n\r\n{1}";
			if (formFields != null)
			{
				foreach (object key in formFields.Keys)
				{
					string text2 = Conversions.ToString(key);
					string s = string.Format(format, text2, formFields[text2]);
					byte[] bytes3 = Encoding.UTF8.GetBytes(s);
					stream.Write(bytes3, 0, bytes3.Length);
				}
			}
			string format2 = "Content-Disposition: form-data; name=\"{0}\"; filename=\"{1}\"\r\nContent-Type: application/octet-stream\r\n\r\n";
			checked
			{
				int num = files.Length - 1;
				for (int i = 0; i <= num; i++)
				{
					stream.Write(bytes, 0, bytes.Length);
					string s2 = string.Format(format2, "images", files[i]);
					byte[] bytes4 = Encoding.UTF8.GetBytes(s2);
					stream.Write(bytes4, 0, bytes4.Length);
					using FileStream fileStream = new FileStream(files[i], FileMode.Open, FileAccess.Read);
					byte[] array = new byte[1024];
					int target = 0;
					while (CSharpImpl.__Assign(ref target, fileStream.Read(array, 0, array.Length)) != 0)
					{
						stream.Write(array, 0, target);
					}
				}
				stream.Write(bytes2, 0, bytes2.Length);
				httpWebRequest.ContentLength = stream.Length;
				using (Stream stream2 = httpWebRequest.GetRequestStream())
				{
					stream.Position = 0L;
					byte[] array2 = new byte[(int)(stream.Length - 1) + 1];
					stream.Read(array2, 0, array2.Length);
					stream.Close();
					stream2.Write(array2, 0, array2.Length);
				}
				using WebResponse webResponse = httpWebRequest.GetResponse();
				Stream responseStream = webResponse.GetResponseStream();
				StreamReader streamReader = new StreamReader(responseStream);
				return streamReader.ReadToEnd();
			}
		}

		private void Button7_Click(object sender, EventArgs e)
		{
			if (con.State == ConnectionState.Open)
			{
				con.Close();
			}
			((Component)this).Dispose();
			((Form)this).Close();
		}

		private void Write_txt_file(string Text, string txt_file_path)
		{
			if (File.Exists(txt_file_path))
			{
				StreamWriter streamWriter = ((ServerComputer)MyProject.Computer).FileSystem.OpenTextFileWriter(txt_file_path, false);
				streamWriter.Write(Text);
				streamWriter.Close();
			}
			else
			{
				StreamWriter streamWriter2 = ((ServerComputer)MyProject.Computer).FileSystem.OpenTextFileWriter(txt_file_path, true);
				streamWriter2.Write(Text);
				streamWriter2.Close();
			}
		}

		public string Read_text_file(string txt_file_path)
		{
			if (File.Exists(txt_file_path))
			{
				return ((ServerComputer)MyProject.Computer).FileSystem.ReadAllText(txt_file_path);
			}
			string result = default;
			return result;
		}

		private void grid3_CellContentClick(object sender, DataGridViewCellEventArgs e)
		{
		}

		private void grid3_KeyDown(object sender, KeyEventArgs e)
		{
			//IL_0002: Unknown result type (might be due to invalid IL or missing references)
			//IL_0009: Invalid comparison between Unknown and I4
			if ((int)e.KeyCode == 32)
			{
				if (Operators.ConditionalCompareObjectEqual(grid3.Rows[((DataGridViewBand)grid3.CurrentRow).Index].Cells[0].Value, true, TextCompare: false))
				{
					grid3.Rows[((DataGridViewBand)grid3.CurrentRow).Index].Cells[0].Value = false;
				}
				else
				{
					grid3.Rows[((DataGridViewBand)grid3.CurrentRow).Index].Cells[0].Value = true;
				}
				Count_selected_cmps();
			}
		}

		private void grid3_Click(object sender, EventArgs e)
		{
		}

		private void Button8_Click(object sender, EventArgs e)
		{
			//IL_000b: Unknown result type (might be due to invalid IL or missing references)
			((Form)MyProject.Forms.Change_details).ShowDialog();
		}

		private void grid3_CellStateChanged(object sender, DataGridViewCellStateChangedEventArgs e)
		{
		}

		private void grid3_CellValueChanged(object sender, DataGridViewCellEventArgs e)
		{
		}

		private void grid3_CellMouseClick(object sender, DataGridViewCellMouseEventArgs e)
		{
		}

		private void Timer1_Tick(object sender, EventArgs e)
		{
			Application.DoEvents();
			if (Operators.CompareString(((ButtonBase)Button3).Text, "", TextCompare: false) == 0)
			{
				((ButtonBase)Button3).Text = Label2.Text;
			}
			else
			{
				((ButtonBase)Button3).Text = "";
			}
		}

		private void grid3_CellEndEdit(object sender, DataGridViewCellEventArgs e)
		{
			Count_selected_cmps();
		}

		private void grid3_MouseClick(object sender, MouseEventArgs e)
		{
			if (Operators.ConditionalCompareObjectEqual(grid3.Rows[((DataGridViewBand)grid3.CurrentRow).Index].Cells[0].Value, true, TextCompare: false))
			{
				grid3.Rows[((DataGridViewBand)grid3.CurrentRow).Index].Cells[0].Value = false;
			}
			else
			{
				grid3.Rows[((DataGridViewBand)grid3.CurrentRow).Index].Cells[0].Value = true;
			}
			Count_selected_cmps();
		}

		private void grid3_MouseUp(object sender, MouseEventArgs e)
		{
			Count_selected_cmps();
		}

		private void grid3_Leave(object sender, EventArgs e)
		{
			Count_selected_cmps();
		}

		private void grid3_CellDoubleClick(object sender, DataGridViewCellEventArgs e)
		{
			Count_selected_cmps();
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
			//IL_001d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0027: Expected Obj, but got Unknown
			//IL_0029: Unknown result type (might be due to invalid IL or missing references)
			//IL_0033: Expected Obj, but got Unknown
			//IL_0035: Unknown result type (might be due to invalid IL or missing references)
			//IL_003f: Expected Obj, but got Unknown
			//IL_0041: Unknown result type (might be due to invalid IL or missing references)
			//IL_004b: Expected Obj, but got Unknown
			//IL_004d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0057: Expected Obj, but got Unknown
			//IL_0059: Unknown result type (might be due to invalid IL or missing references)
			//IL_0063: Expected Obj, but got Unknown
			//IL_0065: Unknown result type (might be due to invalid IL or missing references)
			//IL_006f: Expected Obj, but got Unknown
			//IL_0071: Unknown result type (might be due to invalid IL or missing references)
			//IL_007b: Expected Obj, but got Unknown
			//IL_007d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0087: Expected Obj, but got Unknown
			//IL_0089: Unknown result type (might be due to invalid IL or missing references)
			//IL_0093: Expected Obj, but got Unknown
			//IL_0095: Unknown result type (might be due to invalid IL or missing references)
			//IL_009f: Expected Obj, but got Unknown
			//IL_00a1: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ab: Expected Obj, but got Unknown
			//IL_00ad: Unknown result type (might be due to invalid IL or missing references)
			//IL_00b7: Expected Obj, but got Unknown
			//IL_00b9: Unknown result type (might be due to invalid IL or missing references)
			//IL_00c3: Expected Obj, but got Unknown
			//IL_00c5: Unknown result type (might be due to invalid IL or missing references)
			//IL_00cf: Expected Obj, but got Unknown
			//IL_00d1: Unknown result type (might be due to invalid IL or missing references)
			//IL_00db: Expected Obj, but got Unknown
			//IL_00dd: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e7: Expected Obj, but got Unknown
			//IL_00e9: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f3: Expected Obj, but got Unknown
			//IL_00f5: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ff: Expected Obj, but got Unknown
			//IL_0101: Unknown result type (might be due to invalid IL or missing references)
			//IL_010b: Expected Obj, but got Unknown
			//IL_010d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0117: Expected Obj, but got Unknown
			//IL_0119: Unknown result type (might be due to invalid IL or missing references)
			//IL_0123: Expected Obj, but got Unknown
			//IL_012b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0135: Expected Obj, but got Unknown
			//IL_0172: Unknown result type (might be due to invalid IL or missing references)
			//IL_017c: Expected Obj, but got Unknown
			//IL_0207: Unknown result type (might be due to invalid IL or missing references)
			//IL_0211: Expected Obj, but got Unknown
			//IL_030c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0316: Expected Obj, but got Unknown
			//IL_03c4: Unknown result type (might be due to invalid IL or missing references)
			//IL_03ce: Expected Obj, but got Unknown
			//IL_044d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0457: Expected Obj, but got Unknown
			//IL_05c5: Unknown result type (might be due to invalid IL or missing references)
			//IL_05cf: Expected Obj, but got Unknown
			//IL_0665: Unknown result type (might be due to invalid IL or missing references)
			//IL_066f: Expected Obj, but got Unknown
			//IL_078b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0795: Expected Obj, but got Unknown
			//IL_089b: Unknown result type (might be due to invalid IL or missing references)
			//IL_08a5: Expected Obj, but got Unknown
			//IL_0956: Unknown result type (might be due to invalid IL or missing references)
			//IL_0960: Expected Obj, but got Unknown
			//IL_09fb: Unknown result type (might be due to invalid IL or missing references)
			//IL_0a05: Expected Obj, but got Unknown
			//IL_0aaf: Unknown result type (might be due to invalid IL or missing references)
			//IL_0ab9: Expected Obj, but got Unknown
			//IL_0bd9: Unknown result type (might be due to invalid IL or missing references)
			//IL_0be3: Expected Obj, but got Unknown
			//IL_0c67: Unknown result type (might be due to invalid IL or missing references)
			//IL_0c71: Expected Obj, but got Unknown
			//IL_0d01: Unknown result type (might be due to invalid IL or missing references)
			//IL_0d0b: Expected Obj, but got Unknown
			//IL_0da5: Unknown result type (might be due to invalid IL or missing references)
			//IL_0daf: Expected Obj, but got Unknown
			//IL_1027: Unknown result type (might be due to invalid IL or missing references)
			//IL_1031: Expected Obj, but got Unknown
			components = new Container();
			ComponentResourceManager componentResourceManager = new ComponentResourceManager(typeof(Form1));
			Button1 = new Button();
			Button2 = new Button();
			DataGridView1 = new DataGridView();
			Label1 = new Label();
			Button3 = new Button();
			Button4 = new Button();
			Button5 = new Button();
			pb1 = new ProgressBar();
			btn_add_fields = new Button();
			btn_Fetch_data = new Button();
			Button6 = new Button();
			Label2 = new Label();
			DataGridView2 = new DataGridView();
			Button7 = new Button();
			Label3 = new Label();
			lblDatapath = new Label();
			Label4 = new Label();
			grid3 = new DataGridView();
			Button8 = new Button();
			Lbl_UserID = new Label();
			lbl_sel_cmps = new Label();
			CHKRewrite = new CheckBox();
			Timer1 = new Timer(components);
			((ISupportInitialize)DataGridView1).BeginInit();
			((ISupportInitialize)DataGridView2).BeginInit();
			((ISupportInitialize)grid3).BeginInit();
			((Control)this).SuspendLayout();
			((Control)Button1).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1);
			((Control)Button1).Location = new Point(906, 66);
			((Control)Button1).Name = "Button1";
			((Control)Button1).Size = new Size(148, 46);
			((Control)Button1).TabIndex = 0;
			((ButtonBase)Button1).Text = "&Fetch Data";
			((ButtonBase)Button1).UseVisualStyleBackColor = true;
			((Control)Button1).Visible = false;
			((Control)Button2).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1);
			((Control)Button2).Location = new Point(1075, 66);
			((Control)Button2).Name = "Button2";
			((Control)Button2).Size = new Size(148, 46);
			((Control)Button2).TabIndex = 1;
			((ButtonBase)Button2).Text = "Generate Narration";
			((ButtonBase)Button2).UseVisualStyleBackColor = true;
			((Control)Button2).Visible = false;
			DataGridView1.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
			((Control)DataGridView1).Location = new Point(981, 239);
			((Control)DataGridView1).Name = "DataGridView1";
			((Control)DataGridView1).Size = new Size(474, 240);
			((Control)DataGridView1).TabIndex = 3;
			((Control)DataGridView1).Visible = false;
			((Control)Label1).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Label1).ForeColor = Color.Red;
			((Control)Label1).Location = new Point(164, 464);
			((Control)Label1).Name = "Label1";
			((Control)Label1).Size = new Size(476, 17);
			((Control)Label1).TabIndex = 9;
			Label1.Text = "Label1";
			Label1.TextAlign = (ContentAlignment)16;
			Label1.UseMnemonic = false;
			((ButtonBase)Button3).FlatStyle = (FlatStyle)0;
			((Control)Button3).Font = new Font("Microsoft Sans Serif", 11f, (FontStyle)1);
			((Control)Button3).Location = new Point(10, 464);
			((Control)Button3).Name = "Button3";
			((Control)Button3).Size = new Size(148, 46);
			((Control)Button3).TabIndex = 10;
			((ButtonBase)Button3).Text = "Start Sync Data";
			((ButtonBase)Button3).UseVisualStyleBackColor = true;
			((Control)Button4).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1);
			((Control)Button4).Location = new Point(1242, 66);
			((Control)Button4).Name = "Button4";
			((Control)Button4).Size = new Size(89, 46);
			((Control)Button4).TabIndex = 11;
			((ButtonBase)Button4).Text = "Get Comp List";
			((ButtonBase)Button4).UseVisualStyleBackColor = true;
			((Control)Button4).Visible = false;
			((Control)Button5).Location = new Point(1532, 22);
			((Control)Button5).Name = "Button5";
			((Control)Button5).Size = new Size(102, 34);
			((Control)Button5).TabIndex = 12;
			((ButtonBase)Button5).Text = "Button5";
			((ButtonBase)Button5).UseVisualStyleBackColor = true;
			((Control)Button5).Visible = false;
			((Control)pb1).Location = new Point(317, 484);
			((Control)pb1).Name = "pb1";
			((Control)pb1).Size = new Size(331, 24);
			pb1.Style = (ProgressBarStyle)1;
			((Control)pb1).TabIndex = 13;
			((Control)pb1).Visible = false;
			((Control)btn_add_fields).Font = new Font("Verdana", 8.5f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)btn_add_fields).Location = new Point(1337, 22);
			((Control)btn_add_fields).Name = "btn_add_fields";
			((Control)btn_add_fields).Size = new Size(120, 88);
			((Control)btn_add_fields).TabIndex = 15;
			((ButtonBase)btn_add_fields).Text = "Fields availibility Check and if not existing add them";
			((ButtonBase)btn_add_fields).UseCompatibleTextRendering = true;
			((ButtonBase)btn_add_fields).UseVisualStyleBackColor = true;
			((Control)btn_add_fields).Visible = false;
			((Control)btn_Fetch_data).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1);
			((Control)btn_Fetch_data).Location = new Point(1463, 27);
			((Control)btn_Fetch_data).Name = "btn_Fetch_data";
			((Control)btn_Fetch_data).Size = new Size(82, 42);
			((Control)btn_Fetch_data).TabIndex = 16;
			((ButtonBase)btn_Fetch_data).Text = "&Fetch Data";
			((ButtonBase)btn_Fetch_data).UseCompatibleTextRendering = true;
			((ButtonBase)btn_Fetch_data).UseVisualStyleBackColor = true;
			((Control)btn_Fetch_data).Visible = false;
			((Control)Button6).Location = new Point(1567, 75);
			((Control)Button6).Name = "Button6";
			((Control)Button6).Size = new Size(67, 31);
			((Control)Button6).TabIndex = 17;
			((ButtonBase)Button6).Text = "Button6";
			((ButtonBase)Button6).UseVisualStyleBackColor = true;
			((Control)Button6).Visible = false;
			Label2.AutoSize = true;
			((Control)Label2).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Label2).Location = new Point(591, 50);
			((Control)Label2).Name = "Label2";
			((Control)Label2).Size = new Size(57, 17);
			((Control)Label2).TabIndex = 18;
			Label2.Text = "Label2";
			Label2.TextAlign = (ContentAlignment)32;
			((Control)Label2).Visible = false;
			DataGridView2.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
			((Control)DataGridView2).Location = new Point(937, 226);
			((Control)DataGridView2).Name = "DataGridView2";
			((Control)DataGridView2).Size = new Size(188, 342);
			((Control)DataGridView2).TabIndex = 19;
			((Control)DataGridView2).Visible = false;
			((ButtonBase)Button7).FlatStyle = (FlatStyle)0;
			((Control)Button7).Font = new Font("Microsoft Sans Serif", 11f, (FontStyle)1);
			((Control)Button7).Location = new Point(654, 488);
			((Control)Button7).Name = "Button7";
			((Control)Button7).Size = new Size(135, 27);
			((Control)Button7).TabIndex = 21;
			((ButtonBase)Button7).Text = "Exit";
			((ButtonBase)Button7).UseVisualStyleBackColor = true;
			((Control)Label3).BackColor = Color.FromArgb(255, 255, 192);
			Label3.BorderStyle = (BorderStyle)1;
			((Control)Label3).Font = new Font("Microsoft Sans Serif", 17f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Label3).ForeColor = Color.Blue;
			((Control)Label3).Location = new Point(5, 7);
			((Control)Label3).Name = "Label3";
			((Control)Label3).Size = new Size(784, 40);
			((Control)Label3).TabIndex = 22;
			Label3.Text = "Select Firms For Synchronize Data to Your Android Device";
			Label3.TextAlign = (ContentAlignment)32;
			lblDatapath.AutoSize = true;
			((Control)lblDatapath).Font = new Font("Microsoft Sans Serif", 11f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)lblDatapath).ForeColor = Color.Red;
			((Control)lblDatapath).Location = new Point(5, 71);
			((Control)lblDatapath).Name = "lblDatapath";
			((Control)lblDatapath).Size = new Size(57, 18);
			((Control)lblDatapath).TabIndex = 23;
			lblDatapath.Text = "Label4";
			lblDatapath.TextAlign = (ContentAlignment)32;
			Label4.AutoSize = true;
			((Control)Label4).BackColor = Color.Transparent;
			((Control)Label4).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Label4).ForeColor = Color.Blue;
			((Control)Label4).Location = new Point(164, 487);
			((Control)Label4).Name = "Label4";
			((Control)Label4).Size = new Size(51, 15);
			((Control)Label4).TabIndex = 24;
			Label4.Text = "Label4";
			Label4.TextAlign = (ContentAlignment)32;
			grid3.AllowUserToAddRows = false;
			grid3.AllowUserToDeleteRows = false;
			((Control)grid3).Location = new Point(5, 95);
			((Control)grid3).Name = "grid3";
			grid3.ReadOnly = true;
			grid3.RowHeadersVisible = false;
			((Control)grid3).Size = new Size(786, 363);
			((Control)grid3).TabIndex = 20;
			((ButtonBase)Button8).FlatStyle = (FlatStyle)0;
			((Control)Button8).Font = new Font("Microsoft Sans Serif", 11f, (FontStyle)1);
			((Control)Button8).Location = new Point(654, 459);
			((Control)Button8).Name = "Button8";
			((Control)Button8).Size = new Size(135, 28);
			((Control)Button8).TabIndex = 25;
			((ButtonBase)Button8).Text = "Change Info";
			((ButtonBase)Button8).UseVisualStyleBackColor = true;
			((Control)Lbl_UserID).Font = new Font("Times New Roman", 11f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Lbl_UserID).ForeColor = Color.Red;
			((Control)Lbl_UserID).Location = new Point(698, 47);
			((Control)Lbl_UserID).Name = "Lbl_UserID";
			((Control)Lbl_UserID).Size = new Size(91, 18);
			((Control)Lbl_UserID).TabIndex = 26;
			Lbl_UserID.Text = "Lbl_UserID";
			Lbl_UserID.TextAlign = (ContentAlignment)64;
			((Control)lbl_sel_cmps).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)lbl_sel_cmps).ForeColor = Color.Green;
			((Control)lbl_sel_cmps).Location = new Point(4, 46);
			((Control)lbl_sel_cmps).Name = "lbl_sel_cmps";
			((Control)lbl_sel_cmps).Size = new Size(785, 18);
			((Control)lbl_sel_cmps).TabIndex = 27;
			lbl_sel_cmps.Text = "Selected  Companies";
			lbl_sel_cmps.TextAlign = (ContentAlignment)32;
			((ButtonBase)CHKRewrite).AutoSize = true;
			((Control)CHKRewrite).Font = new Font("Microsoft Sans Serif", 8f, (FontStyle)1);
			((Control)CHKRewrite).ForeColor = Color.Red;
			((Control)CHKRewrite).Location = new Point(7, 49);
			((Control)CHKRewrite).Name = "CHKRewrite";
			((Control)CHKRewrite).Size = new Size(118, 17);
			((Control)CHKRewrite).TabIndex = 28;
			((ButtonBase)CHKRewrite).Text = "Rewrite All Data";
			((ButtonBase)CHKRewrite).UseVisualStyleBackColor = true;
			Timer1.Interval = 500;
			((ContainerControl)this).AutoScaleDimensions = new SizeF(6f, 13f);
			((ContainerControl)this).AutoScaleMode = (AutoScaleMode)1;
			((Form)this).BackColor = Color.FromArgb(225, 255, 255);
			((Form)this).ClientSize = new Size(798, 526);
			((Control)this).Controls.Add((Control)(object)Lbl_UserID);
			((Control)this).Controls.Add((Control)(object)CHKRewrite);
			((Control)this).Controls.Add((Control)(object)Button8);
			((Control)this).Controls.Add((Control)(object)Label4);
			((Control)this).Controls.Add((Control)(object)lblDatapath);
			((Control)this).Controls.Add((Control)(object)Label3);
			((Control)this).Controls.Add((Control)(object)DataGridView1);
			((Control)this).Controls.Add((Control)(object)Button7);
			((Control)this).Controls.Add((Control)(object)grid3);
			((Control)this).Controls.Add((Control)(object)Label2);
			((Control)this).Controls.Add((Control)(object)Button6);
			((Control)this).Controls.Add((Control)(object)btn_Fetch_data);
			((Control)this).Controls.Add((Control)(object)btn_add_fields);
			((Control)this).Controls.Add((Control)(object)pb1);
			((Control)this).Controls.Add((Control)(object)Button5);
			((Control)this).Controls.Add((Control)(object)Button4);
			((Control)this).Controls.Add((Control)(object)Button3);
			((Control)this).Controls.Add((Control)(object)Label1);
			((Control)this).Controls.Add((Control)(object)Button2);
			((Control)this).Controls.Add((Control)(object)Button1);
			((Control)this).Controls.Add((Control)(object)DataGridView2);
			((Control)this).Controls.Add((Control)(object)lbl_sel_cmps);
			((Form)this).FormBorderStyle = (FormBorderStyle)5;
			((Form)this).Icon = (Icon)componentResourceManager.GetObject("$this.Icon");
			((Form)this).KeyPreview = true;
			((Form)this).MaximizeBox = false;
			((Form)this).MinimizeBox = false;
			((Control)this).Name = "Form1";
			((Form)this).StartPosition = (FormStartPosition)1;
			((Form)this).Text = "Form1";
			((ISupportInitialize)DataGridView1).EndInit();
			((ISupportInitialize)DataGridView2).EndInit();
			((ISupportInitialize)grid3).EndInit();
			((Control)this).ResumeLayout(false);
			((Control)this).PerformLayout();
		}
	}
	[StandardModule]
	internal sealed class Module1
	{
		public static string Title_msg;

		public static string URL;

		public static string userid;

		public static string user_email;

		public static string user_mobile;

		public static string db_path;

		private static bool local_run = false;

		public static bool CheckForInternetConnection()
		{
			URL = "http://kamrasoftwares.com/bahikhata/";
			if (((ServerComputer)MyProject.Computer).Network.IsAvailable)
			{
				return true;
			}
			return false;
		}

		[STAThread]
		public static void Main(string[] args)
		{
			//IL_001a: Unknown result type (might be due to invalid IL or missing references)
			//IL_019e: Unknown result type (might be due to invalid IL or missing references)
			local_run = false;
			if (local_run)
			{
				((Form)MyProject.Forms.Reg_User).ShowDialog();
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
						((Form)MyProject.Forms.Reg_User).ShowDialog();
					}
					else
					{
						Interaction.MsgBox("Invalid Licence ID Found", MsgBoxStyle.Critical, "Kamra Softwares");
						Application.Exit();
					}
				}
			}
		}
	}
	[DesignerGenerated]
	public class Reg_User : Form
	{
		private IContainer components;

		private OleDbConnection con;

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

		[field: AccessedThroughProperty("Label1")]
		internal virtual Label Label1
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("txtUserMobile")]
		internal virtual TextBox txtUserMobile
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("txt_UserEmail")]
		internal virtual TextBox txt_UserEmail
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

		[field: AccessedThroughProperty("GroupBox2")]
		internal virtual GroupBox GroupBox2
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("GroupBox1")]
		internal virtual GroupBox GroupBox1
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
				EventHandler eventHandler = Button3_Click_1;
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

		[field: AccessedThroughProperty("Button4")]
		internal virtual Button Button4
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			[CompilerGenerated]
			set
			{
				EventHandler eventHandler = Button4_Click_1;
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

		[field: AccessedThroughProperty("txtPassword")]
		internal virtual TextBox txtPassword
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		[field: AccessedThroughProperty("Label3")]
		internal virtual Label Label3
		{
			get;
			[MethodImpl(MethodImplOptions.Synchronized)]
			set;
		}

		public Reg_User()
		{
			//IL_001b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0025: Expected Obj, but got Unknown
			((Form)this).Load += Reg_User_Load;
			con = new OleDbConnection();
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
			//IL_00de: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e8: Expected Obj, but got Unknown
			//IL_0172: Unknown result type (might be due to invalid IL or missing references)
			//IL_017c: Expected Obj, but got Unknown
			//IL_0202: Unknown result type (might be due to invalid IL or missing references)
			//IL_020c: Expected Obj, but got Unknown
			//IL_027a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0284: Expected Obj, but got Unknown
			//IL_02f1: Unknown result type (might be due to invalid IL or missing references)
			//IL_02fb: Expected Obj, but got Unknown
			//IL_038e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0398: Expected Obj, but got Unknown
			//IL_05c8: Unknown result type (might be due to invalid IL or missing references)
			//IL_05d2: Expected Obj, but got Unknown
			//IL_065b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0665: Expected Obj, but got Unknown
			//IL_06e0: Unknown result type (might be due to invalid IL or missing references)
			//IL_06ea: Expected Obj, but got Unknown
			//IL_0775: Unknown result type (might be due to invalid IL or missing references)
			//IL_077f: Expected Obj, but got Unknown
			//IL_0873: Unknown result type (might be due to invalid IL or missing references)
			//IL_087d: Expected Obj, but got Unknown
			ComponentResourceManager componentResourceManager = new ComponentResourceManager(typeof(Reg_User));
			Button1 = new Button();
			Label1 = new Label();
			txtUserMobile = new TextBox();
			txt_UserEmail = new TextBox();
			Label2 = new Label();
			Button2 = new Button();
			GroupBox2 = new GroupBox();
			GroupBox1 = new GroupBox();
			Button3 = new Button();
			Button4 = new Button();
			txtPassword = new TextBox();
			Label3 = new Label();
			((Control)GroupBox2).SuspendLayout();
			((Control)GroupBox1).SuspendLayout();
			((Control)this).SuspendLayout();
			((ButtonBase)Button1).FlatStyle = (FlatStyle)0;
			((Control)Button1).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Button1).Location = new Point(130, 95);
			((Control)Button1).Name = "Button1";
			((Control)Button1).Size = new Size(92, 31);
			((Control)Button1).TabIndex = 2;
			((ButtonBase)Button1).Text = "Register";
			((ButtonBase)Button1).UseVisualStyleBackColor = true;
			Label1.AutoSize = true;
			((Control)Label1).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Label1).ForeColor = Color.FromArgb(192, 0, 0);
			((Control)Label1).Location = new Point(13, 21);
			((Control)Label1).Name = "Label1";
			((Control)Label1).Size = new Size(178, 17);
			((Control)Label1).TabIndex = 1;
			Label1.Text = "Enter Your Mobile No. :";
			((Control)txtUserMobile).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)txtUserMobile).Location = new Point(171, 19);
			((TextBoxBase)txtUserMobile).MaxLength = 10;
			((Control)txtUserMobile).Name = "txtUserMobile";
			((Control)txtUserMobile).Size = new Size(199, 23);
			((Control)txtUserMobile).TabIndex = 0;
			((Control)txt_UserEmail).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)txt_UserEmail).Location = new Point(171, 48);
			((Control)txt_UserEmail).Name = "txt_UserEmail";
			((Control)txt_UserEmail).Size = new Size(314, 23);
			((Control)txt_UserEmail).TabIndex = 1;
			Label2.AutoSize = true;
			((Control)Label2).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Label2).ForeColor = Color.FromArgb(192, 0, 0);
			((Control)Label2).Location = new Point(22, 50);
			((Control)Label2).Name = "Label2";
			((Control)Label2).Size = new Size(166, 17);
			((Control)Label2).TabIndex = 3;
			Label2.Text = "Enter Your E-mail ID :";
			((ButtonBase)Button2).FlatStyle = (FlatStyle)0;
			((Control)Button2).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Button2).Location = new Point(245, 95);
			((Control)Button2).Name = "Button2";
			((Control)Button2).Size = new Size(92, 31);
			((Control)Button2).TabIndex = 3;
			((ButtonBase)Button2).Text = "Exit";
			((ButtonBase)Button2).UseVisualStyleBackColor = true;
			((Control)GroupBox2).Controls.Add((Control)(object)Button1);
			((Control)GroupBox2).Controls.Add((Control)(object)Button2);
			((Control)GroupBox2).Controls.Add((Control)(object)Label1);
			((Control)GroupBox2).Controls.Add((Control)(object)txt_UserEmail);
			((Control)GroupBox2).Controls.Add((Control)(object)Label2);
			((Control)GroupBox2).Controls.Add((Control)(object)txtUserMobile);
			((Control)GroupBox2).Location = new Point(12, 12);
			((Control)GroupBox2).Name = "GroupBox2";
			((Control)GroupBox2).Size = new Size(512, 148);
			((Control)GroupBox2).TabIndex = 5;
			GroupBox2.TabStop = false;
			((Control)GroupBox1).Controls.Add((Control)(object)Button3);
			((Control)GroupBox1).Controls.Add((Control)(object)Button4);
			((Control)GroupBox1).Controls.Add((Control)(object)txtPassword);
			((Control)GroupBox1).Controls.Add((Control)(object)Label3);
			((Control)GroupBox1).Location = new Point(8, 179);
			((Control)GroupBox1).Name = "GroupBox1";
			((Control)GroupBox1).Size = new Size(512, 103);
			((Control)GroupBox1).TabIndex = 6;
			GroupBox1.TabStop = false;
			((Control)GroupBox1).Visible = false;
			((ButtonBase)Button3).FlatStyle = (FlatStyle)0;
			((Control)Button3).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Button3).Location = new Point(270, 57);
			((Control)Button3).Name = "Button3";
			((Control)Button3).Size = new Size(81, 27);
			((Control)Button3).TabIndex = 9;
			((ButtonBase)Button3).Text = "Exit";
			((ButtonBase)Button3).UseVisualStyleBackColor = true;
			((ButtonBase)Button4).FlatStyle = (FlatStyle)0;
			((Control)Button4).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)Button4).Location = new Point(155, 57);
			((Control)Button4).Name = "Button4";
			((Control)Button4).Size = new Size(92, 27);
			((Control)Button4).TabIndex = 8;
			((ButtonBase)Button4).Text = "OK";
			((ButtonBase)Button4).UseVisualStyleBackColor = true;
			((Control)txtPassword).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1);
			((Control)txtPassword).Location = new Point(223, 18);
			((TextBoxBase)txtPassword).MaxLength = 10;
			((Control)txtPassword).Name = "txtPassword";
			txtPassword.PasswordChar = '*';
			((Control)txtPassword).Size = new Size(199, 23);
			((Control)txtPassword).TabIndex = 6;
			Label3.AutoSize = true;
			((Control)Label3).Font = new Font("Microsoft Sans Serif", 10f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			((Control)Label3).ForeColor = Color.FromArgb(192, 0, 0);
			((Control)Label3).Location = new Point(91, 20);
			((Control)Label3).Name = "Label3";
			((Control)Label3).Size = new Size(131, 17);
			((Control)Label3).TabIndex = 7;
			Label3.Text = "Enter Password :";
			((ContainerControl)this).AutoScaleDimensions = new SizeF(6f, 13f);
			((ContainerControl)this).AutoScaleMode = (AutoScaleMode)1;
			((Form)this).BackColor = Color.FromArgb(225, 255, 255);
			((Form)this).ClientSize = new Size(545, 292);
			((Control)this).Controls.Add((Control)(object)GroupBox1);
			((Control)this).Controls.Add((Control)(object)GroupBox2);
			((Form)this).Icon = (Icon)componentResourceManager.GetObject("$this.Icon");
			((Form)this).MaximizeBox = false;
			((Form)this).MinimizeBox = false;
			((Control)this).Name = "Reg_User";
			((Form)this).StartPosition = (FormStartPosition)1;
			((Form)this).Text = "Registration For Your Android Device";
			((Control)GroupBox2).ResumeLayout(false);
			((Control)GroupBox2).PerformLayout();
			((Control)GroupBox1).ResumeLayout(false);
			((Control)GroupBox1).PerformLayout();
			((Control)this).ResumeLayout(false);
		}

		private void Button1_Click(object sender, EventArgs e)
		{
			//IL_010c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0113: Expected Obj, but got Unknown
			//IL_0137: Unknown result type (might be due to invalid IL or missing references)
			if (Module1.CheckForInternetConnection())
			{
				string text = "pc";
				string text2 = txt_UserEmail.Text;
				string text3 = txtUserMobile.Text;
				string args = "{\"mobileno\":\"" + text3 + "\",\"email\":\"" + text2 + "\",\"pc\":\"" + text + "\"}";
				string url = Module1.URL + "web-service/index.php?service=user_registerpc";
				upload_data_1(url, args);
				if (Operators.CompareString(Module1.userid, "", TextCompare: false) != 0)
				{
					string text4 = toBase64(txt_UserEmail.Text);
					string text5 = toBase64(txtUserMobile.Text);
					string text6 = "INSERT INTO LabourSettings (LabourOnBag, LabourOnWeight,UserID) Values('" + text4 + "','" + text5 + "','" + Module1.userid + "')";
					OleDbCommand val = new OleDbCommand(text6, con);
					val.ExecuteNonQuery();
					Interaction.MsgBox("User Registered Successfully", MsgBoxStyle.Information, Module1.Title_msg);
					((Form)MyProject.Forms.Form1).ShowDialog();
				}
			}
			else
			{
				Interaction.MsgBox("Please Check your internet connection", MsgBoxStyle.Information, Module1.Title_msg);
			}
		}

		public string toBase64(string s)
		{
			byte[] bytes = Encoding.ASCII.GetBytes(s);
			return Convert.ToBase64String(bytes).ToString();
		}

		private void upload_data_1(string url, string args)
		{
			Uri uri = new Uri(url);
			byte[] bytes = Encoding.UTF8.GetBytes(args);
			string text = SendRequest(uri, bytes, "application/json", "POST");
			JObject jObject = default;
			if (Operators.CompareString(text, "", TextCompare: false) != 0)
			{
				jObject = JObject.Parse(text);
			}
			if (Operators.CompareString(jObject["success"].ToString(), "1", TextCompare: false) == 0)
			{
				Module1.userid = jObject["userid"].ToString();
				Module1.userid = Module1.userid.Replace("\"", "");
			}
			else if (Operators.CompareString(jObject["success"].ToString(), "2", TextCompare: false) == 0)
			{
				((Control)GroupBox1).Visible = true;
				Point location = new Point(10, 10);
				((Control)GroupBox2).Visible = false;
				((Control)GroupBox1).Visible = true;
				((Control)GroupBox1).Location = location;
				((Control)txtPassword).Select();
			}
			else
			{
				Interaction.MsgBox(jObject["msg"].ToString());
			}
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

		private void Button2_Click(object sender, EventArgs e)
		{
			Application.Exit();
		}

		public string To_MD5(string s)
		{
			StringBuilder stringBuilder = new StringBuilder();
			byte[] bytes = Encoding.Default.GetBytes(s);
			bytes = MD5.Create().ComputeHash(bytes);
			checked
			{
				int num = bytes.Length - 1;
				for (int i = 0; i <= num; i++)
				{
					stringBuilder.Append(bytes[i].ToString("x2"));
				}
				return stringBuilder.ToString();
			}
		}

		public void conn()
		{
			if (con.State == ConnectionState.Open)
			{
				con.Close();
			}
			string startupPath = Application.StartupPath;
			string text = "";
			string text2 = Application.StartupPath + "\\control.lsp";
			con.ConnectionString = "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + text2 + ";Jet OLEDB:Database Password=jay_hanuman;";
			con.Open();
		}

		private void Reg_User_Load(object sender, EventArgs e)
		{
			//IL_014e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0155: Expected Obj, but got Unknown
			//IL_0063: Unknown result type (might be due to invalid IL or missing references)
			//IL_0069: Expected Obj, but got Unknown
			//IL_0133: Unknown result type (might be due to invalid IL or missing references)
			((Control)this).Height = 210;
			((Control)this).Width = 550;
			conn();
			DataTable oleDbSchemaTable = con.GetOleDbSchemaTable(OleDbSchemaGuid.Columns, new object[3] { null, null, "LabourSettings" });
			if (oleDbSchemaTable.Rows.Count != 0)
			{
				string text = "SELECT * from LabourSettings";
				OleDbDataAdapter val = new OleDbDataAdapter(text, con);
				DataSet dataSet = new DataSet();
				((DbDataAdapter)(object)val).Fill(dataSet);
				if (dataSet.Tables[0].Rows.Count > 0)
				{
					Module1.user_email = dataSet.Tables[0].Rows[0][0].ToString();
					Module1.user_mobile = dataSet.Tables[0].Rows[0][1].ToString();
					Module1.user_email = DecodeBase64(Module1.user_email);
					Module1.user_mobile = DecodeBase64(Module1.user_mobile);
					txt_UserEmail.Text = Module1.user_email;
					txtUserMobile.Text = Module1.user_mobile;
					((Form)MyProject.Forms.Form1).ShowDialog();
				}
			}
			else
			{
				string text2 = "CREATE TABLE LabourSettings (LabourOnBag Text, LabourOnWeight Text,userID Long)";
				OleDbCommand val2 = new OleDbCommand(text2, con);
				val2.ExecuteNonQuery();
			}
		}

		public string DecodeBase64(string input)
		{
			return Encoding.UTF8.GetString(Convert.FromBase64String(input));
		}

		private void Button3_Click(object sender, EventArgs e)
		{
			((Component)this).Dispose();
			((Form)this).Close();
		}

		private void Button4_Click_1(object sender, EventArgs e)
		{
			//IL_0145: Unknown result type (might be due to invalid IL or missing references)
			//IL_014c: Expected Obj, but got Unknown
			//IL_0170: Unknown result type (might be due to invalid IL or missing references)
			if (Module1.CheckForInternetConnection())
			{
				string text = "pc";
				string text2 = txt_UserEmail.Text;
				string text3 = txtUserMobile.Text;
				string text4 = To_MD5(txtPassword.Text);
				string args = "{\"mobileno\":\"" + text3 + "\",\"email\":\"" + text2 + "\",\"password\":\"" + text4 + "\",\"pc\":\"" + text + "\"}";
				string url = Module1.URL + "web-service/index.php?service=user_registerpc";
				upload_data_1(url, args);
				if (Operators.CompareString(Module1.userid, "", TextCompare: false) == 0)
				{
					Interaction.MsgBox("Please Check your internet connection", MsgBoxStyle.Information, Module1.Title_msg);
					Application.Exit();
					return;
				}
				string text5 = toBase64(txt_UserEmail.Text);
				string text6 = toBase64(txtUserMobile.Text);
				string text7 = "INSERT INTO LabourSettings (LabourOnBag, LabourOnWeight,UserID) Values('" + text5 + "','" + text6 + "','" + Module1.userid + "')";
				OleDbCommand val = new OleDbCommand(text7, con);
				val.ExecuteNonQuery();
				Interaction.MsgBox("User Registered Successfully", MsgBoxStyle.Information, Module1.Title_msg);
				((Form)MyProject.Forms.Form1).ShowDialog();
			}
		}

		private void Button3_Click_1(object sender, EventArgs e)
		{
			Application.Exit();
		}
	}
}
