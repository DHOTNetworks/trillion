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
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Resources;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using System.Xml.Linq;
using MatchGSTR2.My;
using Microsoft.Office.Interop.Excel;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.ApplicationServices;
using Microsoft.VisualBasic.CompilerServices;
using Microsoft.VisualBasic.Devices;
using Microsoft.Win32;

[assembly: AssemblyCompany("")]
[assembly: AssemblyProduct("Kamra Softwares")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyTrademark("")]
[assembly: CompilationRelaxations(8)]
[assembly: ComVisible(false)]
[assembly: AssemblyCopyright("Copyright ©  2020")]
[assembly: Debuggable(DebuggableAttribute.DebuggingModes.Default | DebuggableAttribute.DebuggingModes.DisableOptimizations | DebuggableAttribute.DebuggingModes.IgnoreSymbolStoreSequencePoints | DebuggableAttribute.DebuggingModes.EnableEditAndContinue)]
[assembly: AssemblyTitle("GSTR2 Excel")]
[assembly: RuntimeCompatibility(WrapNonExceptionThrows = true)]
[assembly: TargetFramework(".NETFramework,Version=v4.0,Profile=Client", FrameworkDisplayName = ".NET Framework 4 Client Profile")]
[assembly: AssemblyFileVersion("1.0.0.0")]
[assembly: Guid("0d03bb4c-1c16-4cfc-98d4-7b82ebc60c18")]
[assembly: AssemblyVersion("1.0.0.0")]
namespace MatchGSTR2.My
{
	[EditorBrowsable(EditorBrowsableState.Never)]
	[GeneratedCode("MyTemplate", "10.0.0.0")]
	internal class MyApplication : ConsoleApplicationBase
	{
		[DebuggerNonUserCode]
		public MyApplication()
		{
		}
	}
	[EditorBrowsable(EditorBrowsableState.Never)]
	[GeneratedCode("MyTemplate", "10.0.0.0")]
	internal class MyComputer : Computer
	{
		[DebuggerHidden]
		[EditorBrowsable(EditorBrowsableState.Never)]
		public MyComputer()
		{
		}
	}
	[GeneratedCode("MyTemplate", "10.0.0.0")]
	[StandardModule]
	[HideModuleName]
	internal sealed class MyProject
	{
		[MyGroupCollection("System.Windows.Forms.Form", "Create__Instance__", "Dispose__Instance__", "My.MyProject.Forms")]
		[EditorBrowsable(EditorBrowsableState.Never)]
		internal sealed class MyForms
		{
			public Form1 m_Form1;

			public Form2 m_Form2;

			[ThreadStatic]
			private static Hashtable m_FormBeingCreated;

			public Form1 Form1
			{
				[DebuggerNonUserCode]
				get
				{
					m_Form1 = Create__Instance__(m_Form1);
					return m_Form1;
				}
				[DebuggerNonUserCode]
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
				[DebuggerNonUserCode]
				get
				{
					m_Form2 = Create__Instance__(m_Form2);
					return m_Form2;
				}
				[DebuggerNonUserCode]
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
				if (Instance == null || (((Control)Instance).IsDisposed ? true : false))
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

			[EditorBrowsable(EditorBrowsableState.Never)]
			[DebuggerHidden]
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
			[DebuggerHidden]
			[EditorBrowsable(EditorBrowsableState.Never)]
			public override bool Equals(object o)
			{
				return base.Equals(RuntimeHelpers.GetObjectValue(o));
			}

			[DebuggerHidden]
			[EditorBrowsable(EditorBrowsableState.Never)]
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

			[DebuggerHidden]
			[EditorBrowsable(EditorBrowsableState.Never)]
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

			[EditorBrowsable(EditorBrowsableState.Never)]
			[DebuggerHidden]
			public MyWebServices()
			{
			}
		}

		[ComVisible(false)]
		[EditorBrowsable(EditorBrowsableState.Never)]
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
	[CompilerGenerated]
	[DebuggerNonUserCode]
	[EditorBrowsable(EditorBrowsableState.Never)]
	internal sealed class InternalXmlHelper
	{
		[EditorBrowsable(EditorBrowsableState.Never)]
		[DebuggerNonUserCode]
		[CompilerGenerated]
		private sealed class RemoveNamespaceAttributesClosure
		{
			private readonly string[] m_inScopePrefixes;

			private readonly XNamespace[] m_inScopeNs;

			private readonly List<XAttribute> m_attributes;

			[EditorBrowsable(EditorBrowsableState.Never)]
			internal RemoveNamespaceAttributesClosure(string[] inScopePrefixes, XNamespace[] inScopeNs, List<XAttribute> attributes)
			{
				m_inScopePrefixes = inScopePrefixes;
				m_inScopeNs = inScopeNs;
				m_attributes = attributes;
			}

			[EditorBrowsable(EditorBrowsableState.Never)]
			internal XElement ProcessXElement(XElement elem)
			{
				return RemoveNamespaceAttributes(m_inScopePrefixes, m_inScopeNs, m_attributes, elem);
			}

			[EditorBrowsable(EditorBrowsableState.Never)]
			internal object ProcessObject(object obj)
			{
				if (obj is XElement e)
				{
					return RemoveNamespaceAttributes(m_inScopePrefixes, m_inScopeNs, m_attributes, e);
				}
				return obj;
			}
		}

		// C# has no syntax for parameterized property 'Value'.
		public static string get_Value(IEnumerable<XElement> source)
		{
			using (IEnumerator<XElement> enumerator = source.GetEnumerator())
			{
				if (enumerator.MoveNext())
				{
					XElement current = enumerator.Current;
					return current.Value;
				}
			}
			return null;
		}

		public static void set_Value(IEnumerable<XElement> source, string value)
		{
			using IEnumerator<XElement> enumerator = source.GetEnumerator();
			if (enumerator.MoveNext())
			{
				XElement current = enumerator.Current;
				current.Value = value;
			}
		}

		// C# has no syntax for parameterized property 'AttributeValue'.
		public static string get_AttributeValue(IEnumerable<XElement> source, XName name)
		{
			using (IEnumerator<XElement> enumerator = source.GetEnumerator())
			{
				if (enumerator.MoveNext())
				{
					XElement current = enumerator.Current;
					return (string?)current.Attribute(name);
				}
			}
			return null;
		}

		public static void set_AttributeValue(IEnumerable<XElement> source, XName name, string value)
		{
			using IEnumerator<XElement> enumerator = source.GetEnumerator();
			if (enumerator.MoveNext())
			{
				XElement current = enumerator.Current;
				current.SetAttributeValue(name, value);
			}
		}

		// C# has no syntax for parameterized property 'AttributeValue'.
		public static string get_AttributeValue(XElement source, XName name)
		{
			return (string?)source.Attribute(name);
		}

		public static void set_AttributeValue(XElement source, XName name, string value)
		{
			source.SetAttributeValue(name, value);
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		private InternalXmlHelper()
		{
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		public static XAttribute CreateAttribute(XName name, object value)
		{
			if (value == null)
			{
				return null;
			}
			return new XAttribute(name, RuntimeHelpers.GetObjectValue(value));
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		public static XAttribute CreateNamespaceAttribute(XName name, XNamespace ns)
		{
			XAttribute xAttribute = new XAttribute(name, ns.NamespaceName);
			xAttribute.AddAnnotation(ns);
			return xAttribute;
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		public static object RemoveNamespaceAttributes(string[] inScopePrefixes, XNamespace[] inScopeNs, List<XAttribute> attributes, object obj)
		{
			if (obj != null)
			{
				if (obj is XElement e)
				{
					return RemoveNamespaceAttributes(inScopePrefixes, inScopeNs, attributes, e);
				}
				if (obj is IEnumerable obj2)
				{
					return RemoveNamespaceAttributes(inScopePrefixes, inScopeNs, attributes, obj2);
				}
			}
			return obj;
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		public static IEnumerable RemoveNamespaceAttributes(string[] inScopePrefixes, XNamespace[] inScopeNs, List<XAttribute> attributes, IEnumerable obj)
		{
			if (obj != null)
			{
				if (obj is IEnumerable<XElement> source)
				{
					return source.Select(new RemoveNamespaceAttributesClosure(inScopePrefixes, inScopeNs, attributes).ProcessXElement);
				}
				return obj.Cast<object>().Select(new RemoveNamespaceAttributesClosure(inScopePrefixes, inScopeNs, attributes).ProcessObject);
			}
			return obj;
		}

		[EditorBrowsable(EditorBrowsableState.Never)]
		public static XElement RemoveNamespaceAttributes(string[] inScopePrefixes, XNamespace[] inScopeNs, List<XAttribute> attributes, XElement e)
		{
			checked
			{
				if (e != null)
				{
					XAttribute xAttribute = e.FirstAttribute;
					while (xAttribute != null)
					{
						XAttribute nextAttribute = xAttribute.NextAttribute;
						if (xAttribute.IsNamespaceDeclaration)
						{
							XNamespace xNamespace = xAttribute.Annotation<XNamespace>();
							string localName = xAttribute.Name.LocalName;
							if ((object)xNamespace != null)
							{
								if ((inScopePrefixes != null && inScopeNs != null) ? true : false)
								{
									int num = inScopePrefixes.Length - 1;
									int num2 = num;
									int num3 = 0;
									while (true)
									{
										int num4 = num3;
										int num5 = num2;
										if (num4 > num5)
										{
											break;
										}
										string value = inScopePrefixes[num3];
										XNamespace xNamespace2 = inScopeNs[num3];
										if (localName.Equals(value))
										{
											if (xNamespace == xNamespace2)
											{
												xAttribute.Remove();
											}
											xAttribute = null;
											break;
										}
										num3++;
									}
								}
								if (xAttribute != null)
								{
									if (attributes != null)
									{
										int num6 = attributes.Count - 1;
										int num7 = num6;
										int num8 = 0;
										while (true)
										{
											int num9 = num8;
											int num5 = num7;
											if (num9 > num5)
											{
												break;
											}
											XAttribute xAttribute2 = attributes[num8];
											string localName2 = xAttribute2.Name.LocalName;
											XNamespace xNamespace3 = xAttribute2.Annotation<XNamespace>();
											if ((object)xNamespace3 != null && localName.Equals(localName2))
											{
												if (xNamespace == xNamespace3)
												{
													xAttribute.Remove();
												}
												xAttribute = null;
												break;
											}
											num8++;
										}
									}
									if (xAttribute != null)
									{
										xAttribute.Remove();
										attributes.Add(xAttribute);
									}
								}
							}
						}
						xAttribute = nextAttribute;
					}
				}
				return e;
			}
		}
	}
}
namespace MatchGSTR2
{
	[DesignerGenerated]
	public class Form1 : Form
	{
		private static List<WeakReference> __ENCList = new List<WeakReference>();

		private IContainer components;

		[AccessedThroughProperty("Button2")]
		private Button _Button2;

		[AccessedThroughProperty("DataGridView1")]
		private DataGridView _DataGridView1;

		[AccessedThroughProperty("Button1")]
		private Button _Button1;

		[AccessedThroughProperty("DataGridView2")]
		private DataGridView _DataGridView2;

		[AccessedThroughProperty("Button3")]
		private Button _Button3;

		[AccessedThroughProperty("DataGridView3")]
		private DataGridView _DataGridView3;

		[AccessedThroughProperty("TxtInfo")]
		private TextBox _TxtInfo;

		[AccessedThroughProperty("Label1")]
		private Label _Label1;

		[AccessedThroughProperty("Button4")]
		private Button _Button4;

		[AccessedThroughProperty("lblBahikhatapath")]
		private Label _lblBahikhatapath;

		[AccessedThroughProperty("lblGSTRpath")]
		private Label _lblGSTRpath;

		[AccessedThroughProperty("PB1")]
		private ProgressBar _PB1;

		[AccessedThroughProperty("PB2")]
		private ProgressBar _PB2;

		[AccessedThroughProperty("lblResultfilepath")]
		private Label _lblResultfilepath;

		[AccessedThroughProperty("Button5")]
		private Button _Button5;

		[AccessedThroughProperty("PB3")]
		private ProgressBar _PB3;

		[AccessedThroughProperty("FolderBrowserDialog1")]
		private FolderBrowserDialog _FolderBrowserDialog1;

		[AccessedThroughProperty("Button7")]
		private Button _Button7;

		[AccessedThroughProperty("grid1")]
		private DataGridView _grid1;

		[AccessedThroughProperty("grid2")]
		private DataGridView _grid2;

		[AccessedThroughProperty("grid3")]
		private DataGridView _grid3;

		[AccessedThroughProperty("Button6")]
		private Button _Button6;

		[AccessedThroughProperty("Label2")]
		private Label _Label2;

		[AccessedThroughProperty("txtIgnoreDays")]
		private TextBox _txtIgnoreDays;

		[AccessedThroughProperty("GroupBox1")]
		private GroupBox _GroupBox1;

		[AccessedThroughProperty("RB_Inv_amt")]
		private RadioButton _RB_Inv_amt;

		[AccessedThroughProperty("RB_Inv_no")]
		private RadioButton _RB_Inv_no;

		[AccessedThroughProperty("CheckBox1")]
		private CheckBox _CheckBox1;

		[AccessedThroughProperty("CheckBox2")]
		private CheckBox _CheckBox2;

		private string type;

		private string type_notes;

		private string path1;

		private string path2;

		private string cursor_file;

		private string file_nm;

		private bool first_type;

		private bool second_type;

		private bool third_type;

		internal virtual Button Button2
		{
			[DebuggerNonUserCode]
			get
			{
				return _Button2;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				EventHandler eventHandler = Button2_Click;
				if (_Button2 != null)
				{
					((Control)_Button2).Click -= eventHandler;
				}
				_Button2 = value;
				if (_Button2 != null)
				{
					((Control)_Button2).Click += eventHandler;
				}
			}
		}

		internal virtual DataGridView DataGridView1
		{
			[DebuggerNonUserCode]
			get
			{
				return _DataGridView1;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_DataGridView1 = value;
			}
		}

		internal virtual Button Button1
		{
			[DebuggerNonUserCode]
			get
			{
				return _Button1;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				EventHandler eventHandler = Button1_Click;
				if (_Button1 != null)
				{
					((Control)_Button1).Click -= eventHandler;
				}
				_Button1 = value;
				if (_Button1 != null)
				{
					((Control)_Button1).Click += eventHandler;
				}
			}
		}

		internal virtual DataGridView DataGridView2
		{
			[DebuggerNonUserCode]
			get
			{
				return _DataGridView2;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_DataGridView2 = value;
			}
		}

		internal virtual Button Button3
		{
			[DebuggerNonUserCode]
			get
			{
				return _Button3;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				EventHandler eventHandler = Button3_Click;
				if (_Button3 != null)
				{
					((Control)_Button3).Click -= eventHandler;
				}
				_Button3 = value;
				if (_Button3 != null)
				{
					((Control)_Button3).Click += eventHandler;
				}
			}
		}

		internal virtual DataGridView DataGridView3
		{
			[DebuggerNonUserCode]
			get
			{
				return _DataGridView3;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_DataGridView3 = value;
			}
		}

		internal virtual TextBox TxtInfo
		{
			[DebuggerNonUserCode]
			get
			{
				return _TxtInfo;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_TxtInfo = value;
			}
		}

		internal virtual Label Label1
		{
			[DebuggerNonUserCode]
			get
			{
				return _Label1;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_Label1 = value;
			}
		}

		internal virtual Button Button4
		{
			[DebuggerNonUserCode]
			get
			{
				return _Button4;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				EventHandler eventHandler = Button4_Click;
				if (_Button4 != null)
				{
					((Control)_Button4).Click -= eventHandler;
				}
				_Button4 = value;
				if (_Button4 != null)
				{
					((Control)_Button4).Click += eventHandler;
				}
			}
		}

		internal virtual Label lblBahikhatapath
		{
			[DebuggerNonUserCode]
			get
			{
				return _lblBahikhatapath;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_lblBahikhatapath = value;
			}
		}

		internal virtual Label lblGSTRpath
		{
			[DebuggerNonUserCode]
			get
			{
				return _lblGSTRpath;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_lblGSTRpath = value;
			}
		}

		internal virtual ProgressBar PB1
		{
			[DebuggerNonUserCode]
			get
			{
				return _PB1;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_PB1 = value;
			}
		}

		internal virtual ProgressBar PB2
		{
			[DebuggerNonUserCode]
			get
			{
				return _PB2;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_PB2 = value;
			}
		}

		internal virtual Label lblResultfilepath
		{
			[DebuggerNonUserCode]
			get
			{
				return _lblResultfilepath;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_lblResultfilepath = value;
			}
		}

		internal virtual Button Button5
		{
			[DebuggerNonUserCode]
			get
			{
				return _Button5;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				EventHandler eventHandler = Button5_Click;
				if (_Button5 != null)
				{
					((Control)_Button5).Click -= eventHandler;
				}
				_Button5 = value;
				if (_Button5 != null)
				{
					((Control)_Button5).Click += eventHandler;
				}
			}
		}

		internal virtual ProgressBar PB3
		{
			[DebuggerNonUserCode]
			get
			{
				return _PB3;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_PB3 = value;
			}
		}

		internal virtual FolderBrowserDialog FolderBrowserDialog1
		{
			[DebuggerNonUserCode]
			get
			{
				return _FolderBrowserDialog1;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_FolderBrowserDialog1 = value;
			}
		}

		internal virtual Button Button7
		{
			[DebuggerNonUserCode]
			get
			{
				return _Button7;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				EventHandler eventHandler = Button7_Click;
				if (_Button7 != null)
				{
					((Control)_Button7).Click -= eventHandler;
				}
				_Button7 = value;
				if (_Button7 != null)
				{
					((Control)_Button7).Click += eventHandler;
				}
			}
		}

		internal virtual DataGridView grid1
		{
			[DebuggerNonUserCode]
			get
			{
				return _grid1;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_grid1 = value;
			}
		}

		internal virtual DataGridView grid2
		{
			[DebuggerNonUserCode]
			get
			{
				return _grid2;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_grid2 = value;
			}
		}

		internal virtual DataGridView grid3
		{
			[DebuggerNonUserCode]
			get
			{
				return _grid3;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_grid3 = value;
			}
		}

		internal virtual Button Button6
		{
			[DebuggerNonUserCode]
			get
			{
				return _Button6;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				EventHandler eventHandler = Button6_Click;
				if (_Button6 != null)
				{
					((Control)_Button6).Click -= eventHandler;
				}
				_Button6 = value;
				if (_Button6 != null)
				{
					((Control)_Button6).Click += eventHandler;
				}
			}
		}

		internal virtual Label Label2
		{
			[DebuggerNonUserCode]
			get
			{
				return _Label2;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_Label2 = value;
			}
		}

		internal virtual TextBox txtIgnoreDays
		{
			[DebuggerNonUserCode]
			get
			{
				return _txtIgnoreDays;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_txtIgnoreDays = value;
			}
		}

		internal virtual GroupBox GroupBox1
		{
			[DebuggerNonUserCode]
			get
			{
				return _GroupBox1;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_GroupBox1 = value;
			}
		}

		internal virtual RadioButton RB_Inv_amt
		{
			[DebuggerNonUserCode]
			get
			{
				return _RB_Inv_amt;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_RB_Inv_amt = value;
			}
		}

		internal virtual RadioButton RB_Inv_no
		{
			[DebuggerNonUserCode]
			get
			{
				return _RB_Inv_no;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_RB_Inv_no = value;
			}
		}

		internal virtual CheckBox CheckBox1
		{
			[DebuggerNonUserCode]
			get
			{
				return _CheckBox1;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_CheckBox1 = value;
			}
		}

		internal virtual CheckBox CheckBox2
		{
			[DebuggerNonUserCode]
			get
			{
				return _CheckBox2;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_CheckBox2 = value;
			}
		}

		[DebuggerNonUserCode]
		public Form1()
		{
			((Form)this).Load += Form1_Load;
			__ENCAddToList(this);
			InitializeComponent();
		}

		[DebuggerNonUserCode]
		private static void __ENCAddToList(object value)
		{
			checked
			{
				lock (__ENCList)
				{
					if (__ENCList.Count == __ENCList.Capacity)
					{
						int num = 0;
						int num2 = __ENCList.Count - 1;
						int num3 = 0;
						while (true)
						{
							int num4 = num3;
							int num5 = num2;
							if (num4 > num5)
							{
								break;
							}
							WeakReference weakReference = __ENCList[num3];
							if (weakReference.IsAlive)
							{
								if (num3 != num)
								{
									__ENCList[num] = __ENCList[num3];
								}
								num++;
							}
							num3++;
						}
						__ENCList.RemoveRange(num, __ENCList.Count - num);
						__ENCList.Capacity = __ENCList.Count;
					}
					__ENCList.Add(new WeakReference(RuntimeHelpers.GetObjectValue(value)));
				}
			}
		}

		[DebuggerNonUserCode]
		protected override void Dispose(bool disposing)
		{
			try
			{
				if ((disposing && components != null) ? true : false)
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
			//IL_01db: Unknown result type (might be due to invalid IL or missing references)
			//IL_01e5: Expected Obj, but got Unknown
			//IL_0307: Unknown result type (might be due to invalid IL or missing references)
			//IL_0311: Expected Obj, but got Unknown
			//IL_0434: Unknown result type (might be due to invalid IL or missing references)
			//IL_043e: Expected Obj, but got Unknown
			//IL_056c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0576: Expected Obj, but got Unknown
			//IL_05f9: Unknown result type (might be due to invalid IL or missing references)
			//IL_0603: Expected Obj, but got Unknown
			//IL_0690: Unknown result type (might be due to invalid IL or missing references)
			//IL_069a: Expected Obj, but got Unknown
			//IL_072b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0735: Expected Obj, but got Unknown
			//IL_07b6: Unknown result type (might be due to invalid IL or missing references)
			//IL_07c0: Expected Obj, but got Unknown
			//IL_0903: Unknown result type (might be due to invalid IL or missing references)
			//IL_090d: Expected Obj, but got Unknown
			//IL_0994: Unknown result type (might be due to invalid IL or missing references)
			//IL_099e: Expected Obj, but got Unknown
			//IL_0a8a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0a94: Expected Obj, but got Unknown
			//IL_0d01: Unknown result type (might be due to invalid IL or missing references)
			//IL_0d0b: Expected Obj, but got Unknown
			//IL_0daa: Unknown result type (might be due to invalid IL or missing references)
			//IL_0db4: Expected Obj, but got Unknown
			//IL_0e3d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0e47: Expected Obj, but got Unknown
			//IL_0f1b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0f25: Expected Obj, but got Unknown
			//IL_1421: Unknown result type (might be due to invalid IL or missing references)
			//IL_142b: Expected Obj, but got Unknown
			ComponentResourceManager componentResourceManager = new ComponentResourceManager(typeof(Form1));
			Button2 = new Button();
			DataGridView1 = new DataGridView();
			Button1 = new Button();
			DataGridView2 = new DataGridView();
			Button3 = new Button();
			DataGridView3 = new DataGridView();
			TxtInfo = new TextBox();
			Label1 = new Label();
			Button4 = new Button();
			lblBahikhatapath = new Label();
			lblGSTRpath = new Label();
			PB1 = new ProgressBar();
			PB2 = new ProgressBar();
			lblResultfilepath = new Label();
			Button5 = new Button();
			PB3 = new ProgressBar();
			FolderBrowserDialog1 = new FolderBrowserDialog();
			Button7 = new Button();
			grid1 = new DataGridView();
			grid2 = new DataGridView();
			grid3 = new DataGridView();
			Button6 = new Button();
			Label2 = new Label();
			txtIgnoreDays = new TextBox();
			GroupBox1 = new GroupBox();
			CheckBox2 = new CheckBox();
			CheckBox1 = new CheckBox();
			RB_Inv_amt = new RadioButton();
			RB_Inv_no = new RadioButton();
			((ISupportInitialize)DataGridView1).BeginInit();
			((ISupportInitialize)DataGridView2).BeginInit();
			((ISupportInitialize)DataGridView3).BeginInit();
			((ISupportInitialize)grid1).BeginInit();
			((ISupportInitialize)grid2).BeginInit();
			((ISupportInitialize)grid3).BeginInit();
			((Control)GroupBox1).SuspendLayout();
			((Control)this).SuspendLayout();
			((Control)Button2).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			Button button = Button2;
			Point location = new Point(5, 3);
			((Control)button).Location = location;
			((Control)Button2).Name = "Button2";
			Button button2 = Button2;
			Size size = new Size(139, 47);
			((Control)button2).Size = size;
			((Control)Button2).TabIndex = 2;
			((ButtonBase)Button2).Text = "Select Bahi-Khata GSTR-2 File";
			((ButtonBase)Button2).UseVisualStyleBackColor = true;
			DataGridView1.AllowUserToAddRows = false;
			DataGridView1.AllowUserToDeleteRows = false;
			DataGridView1.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
			DataGridView dataGridView = DataGridView1;
			location = new Point(664, 174);
			((Control)dataGridView).Location = location;
			((Control)DataGridView1).Name = "DataGridView1";
			DataGridView1.ReadOnly = true;
			DataGridView dataGridView2 = DataGridView1;
			size = new Size(645, 532);
			((Control)dataGridView2).Size = size;
			((Control)DataGridView1).TabIndex = 3;
			((Control)DataGridView1).Visible = false;
			((Control)Button1).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			Button button3 = Button1;
			location = new Point(143, 4);
			((Control)button3).Location = location;
			((Control)Button1).Name = "Button1";
			Button button4 = Button1;
			size = new Size(226, 46);
			((Control)button4).Size = size;
			((Control)Button1).TabIndex = 4;
			((ButtonBase)Button1).Text = "Select GSTR-2A File Received from Advocate/CA/Portal";
			((ButtonBase)Button1).UseVisualStyleBackColor = true;
			DataGridView2.AllowUserToAddRows = false;
			DataGridView2.AllowUserToDeleteRows = false;
			DataGridView2.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
			DataGridView dataGridView3 = DataGridView2;
			location = new Point(24, 162);
			((Control)dataGridView3).Location = location;
			((Control)DataGridView2).Name = "DataGridView2";
			DataGridView2.ReadOnly = true;
			DataGridView dataGridView4 = DataGridView2;
			size = new Size(609, 532);
			((Control)dataGridView4).Size = size;
			((Control)DataGridView2).TabIndex = 5;
			((Control)DataGridView2).Visible = false;
			((Control)Button3).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			Button button5 = Button3;
			location = new Point(369, 4);
			((Control)button5).Location = location;
			((Control)Button3).Name = "Button3";
			Button button6 = Button3;
			size = new Size(129, 46);
			((Control)button6).Size = size;
			((Control)Button3).TabIndex = 6;
			((ButtonBase)Button3).Text = "Compare Invoices (See Result)";
			((ButtonBase)Button3).UseVisualStyleBackColor = true;
			DataGridView3.AllowUserToAddRows = false;
			DataGridView3.AllowUserToDeleteRows = false;
			((Control)DataGridView3).Anchor = (AnchorStyles)15;
			DataGridView3.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
			DataGridView dataGridView5 = DataGridView3;
			location = new Point(5, 141);
			((Control)dataGridView5).Location = location;
			((Control)DataGridView3).Name = "DataGridView3";
			DataGridView3.ReadOnly = true;
			DataGridView3.RowHeadersVisible = false;
			DataGridView dataGridView6 = DataGridView3;
			size = new Size(1268, 418);
			((Control)dataGridView6).Size = size;
			((Control)DataGridView3).TabIndex = 7;
			((Control)TxtInfo).Font = new Font("Microsoft Sans Serif", 9.75f, (FontStyle)1);
			TextBox txtInfo = TxtInfo;
			location = new Point(636, 4);
			((Control)txtInfo).Location = location;
			((Control)TxtInfo).Name = "TxtInfo";
			TextBox txtInfo2 = TxtInfo;
			size = new Size(21, 22);
			((Control)txtInfo2).Size = size;
			((Control)TxtInfo).TabIndex = 8;
			TxtInfo.Text = "1";
			Label1.AutoSize = true;
			((Control)Label1).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1);
			((Control)Label1).ForeColor = Color.Red;
			Label label = Label1;
			location = new Point(504, 7);
			((Control)label).Location = location;
			((Control)Label1).Name = "Label1";
			Label label2 = Label1;
			size = new Size(129, 15);
			((Control)label2).Size = size;
			((Control)Label1).TabIndex = 9;
			Label1.Text = "Ignore Amount Rs.:";
			((Control)Button4).Font = new Font("Microsoft Sans Serif", 9.75f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			Button button7 = Button4;
			location = new Point(660, 3);
			((Control)button7).Location = location;
			((Control)Button4).Name = "Button4";
			Button button8 = Button4;
			size = new Size(122, 47);
			((Control)button8).Size = size;
			((Control)Button4).TabIndex = 10;
			((ButtonBase)Button4).Text = "Export to Excel";
			((ButtonBase)Button4).UseVisualStyleBackColor = true;
			lblBahikhatapath.AutoSize = true;
			((Control)lblBahikhatapath).Font = new Font("Microsoft Sans Serif", 8.5f, (FontStyle)1);
			Label val = lblBahikhatapath;
			location = new Point(5, 55);
			((Control)val).Location = location;
			((Control)lblBahikhatapath).Name = "lblBahikhatapath";
			Label val2 = lblBahikhatapath;
			size = new Size(96, 15);
			((Control)val2).Size = size;
			((Control)lblBahikhatapath).TabIndex = 11;
			lblBahikhatapath.Text = "Ignore Value :";
			lblGSTRpath.AutoSize = true;
			((Control)lblGSTRpath).Font = new Font("Microsoft Sans Serif", 8.5f, (FontStyle)1);
			Label val3 = lblGSTRpath;
			location = new Point(5, 75);
			((Control)val3).Location = location;
			((Control)lblGSTRpath).Name = "lblGSTRpath";
			Label val4 = lblGSTRpath;
			size = new Size(96, 15);
			((Control)val4).Size = size;
			((Control)lblGSTRpath).TabIndex = 12;
			lblGSTRpath.Text = "Ignore Value :";
			ProgressBar pB = PB1;
			location = new Point(5, 54);
			((Control)pB).Location = location;
			((Control)PB1).Name = "PB1";
			ProgressBar pB2 = PB1;
			size = new Size(358, 14);
			((Control)pB2).Size = size;
			((Control)PB1).TabIndex = 13;
			((Control)PB1).Visible = false;
			ProgressBar pB3 = PB2;
			location = new Point(5, 80);
			((Control)pB3).Location = location;
			((Control)PB2).Name = "PB2";
			ProgressBar pB4 = PB2;
			size = new Size(358, 13);
			((Control)pB4).Size = size;
			((Control)PB2).TabIndex = 14;
			((Control)PB2).Visible = false;
			lblResultfilepath.AutoSize = true;
			((Control)lblResultfilepath).Font = new Font("Microsoft Sans Serif", 8.5f, (FontStyle)1);
			((Control)lblResultfilepath).ForeColor = Color.Red;
			Label val5 = lblResultfilepath;
			location = new Point(6, 97);
			((Control)val5).Location = location;
			((Control)lblResultfilepath).Name = "lblResultfilepath";
			Label val6 = lblResultfilepath;
			size = new Size(96, 15);
			((Control)val6).Size = size;
			((Control)lblResultfilepath).TabIndex = 15;
			lblResultfilepath.Text = "Ignore Value :";
			((Control)Button5).Font = new Font("Microsoft Sans Serif", 9.75f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			Button button9 = Button5;
			location = new Point(695, 49);
			((Control)button9).Location = location;
			((Control)Button5).Name = "Button5";
			Button button10 = Button5;
			size = new Size(87, 43);
			((Control)button10).Size = size;
			((Control)Button5).TabIndex = 16;
			((ButtonBase)Button5).Text = "&Exit";
			((ButtonBase)Button5).UseVisualStyleBackColor = true;
			ProgressBar pB5 = PB3;
			location = new Point(369, 94);
			((Control)pB5).Location = location;
			((Control)PB3).Name = "PB3";
			ProgressBar pB6 = PB3;
			size = new Size(155, 19);
			((Control)pB6).Size = size;
			((Control)PB3).TabIndex = 17;
			((Control)PB3).Visible = false;
			((Control)Button7).Font = new Font("Microsoft Sans Serif", 9.75f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			Button button11 = Button7;
			location = new Point(369, 50);
			((Control)button11).Location = location;
			((Control)Button7).Name = "Button7";
			Button button12 = Button7;
			size = new Size(155, 43);
			((Control)button12).Size = size;
			((Control)Button7).TabIndex = 19;
			((ButtonBase)Button7).Text = "Compare Received Debit/Credit Notes";
			((ButtonBase)Button7).UseVisualStyleBackColor = true;
			grid1.AllowUserToAddRows = false;
			grid1.AllowUserToDeleteRows = false;
			grid1.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
			DataGridView val7 = grid1;
			location = new Point(5, 155);
			((Control)val7).Location = location;
			((Control)grid1).Name = "grid1";
			grid1.ReadOnly = true;
			DataGridView val8 = grid1;
			size = new Size(645, 532);
			((Control)val8).Size = size;
			((Control)grid1).TabIndex = 20;
			((Control)grid1).Visible = false;
			grid2.AllowUserToAddRows = false;
			grid2.AllowUserToDeleteRows = false;
			grid2.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
			DataGridView val9 = grid2;
			location = new Point(664, 155);
			((Control)val9).Location = location;
			((Control)grid2).Name = "grid2";
			grid2.ReadOnly = true;
			DataGridView val10 = grid2;
			size = new Size(645, 532);
			((Control)val10).Size = size;
			((Control)grid2).TabIndex = 21;
			((Control)grid2).Visible = false;
			grid3.AllowUserToAddRows = false;
			grid3.AllowUserToDeleteRows = false;
			((Control)grid3).Anchor = (AnchorStyles)15;
			grid3.ColumnHeadersHeightSizeMode = (DataGridViewColumnHeadersHeightSizeMode)2;
			DataGridView val11 = grid3;
			location = new Point(5, 142);
			((Control)val11).Location = location;
			((Control)grid3).Name = "grid3";
			grid3.ReadOnly = true;
			grid3.RowHeadersVisible = false;
			DataGridView val12 = grid3;
			size = new Size(1268, 418);
			((Control)val12).Size = size;
			((Control)grid3).TabIndex = 22;
			((Control)Button6).Font = new Font("Microsoft Sans Serif", 9.75f, (FontStyle)1, (GraphicsUnit)3, (byte)0);
			Button button13 = Button6;
			location = new Point(1001, 12);
			((Control)button13).Location = location;
			((Control)Button6).Name = "Button6";
			Button button14 = Button6;
			size = new Size(122, 43);
			((Control)button14).Size = size;
			((Control)Button6).TabIndex = 23;
			((ButtonBase)Button6).Text = "&Exit";
			((ButtonBase)Button6).UseVisualStyleBackColor = true;
			((Control)Button6).Visible = false;
			Label2.AutoSize = true;
			((Control)Label2).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1);
			((Control)Label2).ForeColor = Color.Red;
			Label label3 = Label2;
			location = new Point(528, 30);
			((Control)label3).Location = location;
			((Control)Label2).Name = "Label2";
			Label label4 = Label2;
			size = new Size(91, 15);
			((Control)label4).Size = size;
			((Control)Label2).TabIndex = 25;
			Label2.Text = "Ignore Days :";
			((Control)txtIgnoreDays).Font = new Font("Microsoft Sans Serif", 9.75f, (FontStyle)1);
			TextBox val13 = txtIgnoreDays;
			location = new Point(621, 27);
			((Control)val13).Location = location;
			((Control)txtIgnoreDays).Name = "txtIgnoreDays";
			TextBox val14 = txtIgnoreDays;
			size = new Size(36, 22);
			((Control)val14).Size = size;
			((Control)txtIgnoreDays).TabIndex = 24;
			txtIgnoreDays.Text = "0";
			((Control)GroupBox1).Controls.Add((Control)(object)CheckBox2);
			((Control)GroupBox1).Controls.Add((Control)(object)CheckBox1);
			((Control)GroupBox1).Controls.Add((Control)(object)RB_Inv_amt);
			((Control)GroupBox1).Controls.Add((Control)(object)RB_Inv_no);
			((Control)GroupBox1).Font = new Font("Microsoft Sans Serif", 9f, (FontStyle)1);
			((Control)GroupBox1).ForeColor = Color.Blue;
			GroupBox groupBox = GroupBox1;
			location = new Point(528, 49);
			((Control)groupBox).Location = location;
			((Control)GroupBox1).Name = "GroupBox1";
			GroupBox groupBox2 = GroupBox1;
			size = new Size(154, 89);
			((Control)groupBox2).Size = size;
			((Control)GroupBox1).TabIndex = 27;
			GroupBox1.TabStop = false;
			GroupBox1.Text = "Compare Parameter";
			((ButtonBase)CheckBox2).AutoSize = true;
			CheckBox2.Checked = true;
			CheckBox2.CheckState = (CheckState)1;
			((Control)CheckBox2).Enabled = false;
			CheckBox checkBox = CheckBox2;
			location = new Point(10, 34);
			((Control)checkBox).Location = location;
			((Control)CheckBox2).Name = "CheckBox2";
			CheckBox checkBox2 = CheckBox2;
			size = new Size(105, 19);
			((Control)checkBox2).Size = size;
			((Control)CheckBox2).TabIndex = 5;
			((ButtonBase)CheckBox2).Text = "Invoice Date";
			((ButtonBase)CheckBox2).UseVisualStyleBackColor = true;
			((ButtonBase)CheckBox1).AutoSize = true;
			CheckBox1.Checked = true;
			CheckBox1.CheckState = (CheckState)1;
			((Control)CheckBox1).Enabled = false;
			CheckBox checkBox3 = CheckBox1;
			location = new Point(10, 16);
			((Control)checkBox3).Location = location;
			((Control)CheckBox1).Name = "CheckBox1";
			CheckBox checkBox4 = CheckBox1;
			size = new Size(75, 19);
			((Control)checkBox4).Size = size;
			((Control)CheckBox1).TabIndex = 4;
			((ButtonBase)CheckBox1).Text = "GST No";
			((ButtonBase)CheckBox1).UseVisualStyleBackColor = true;
			((ButtonBase)RB_Inv_amt).AutoSize = true;
			((Control)RB_Inv_amt).ForeColor = Color.Red;
			RadioButton rB_Inv_amt = RB_Inv_amt;
			location = new Point(10, 66);
			((Control)rB_Inv_amt).Location = location;
			((Control)RB_Inv_amt).Name = "RB_Inv_amt";
			RadioButton rB_Inv_amt2 = RB_Inv_amt;
			size = new Size(122, 19);
			((Control)rB_Inv_amt2).Size = size;
			((Control)RB_Inv_amt).TabIndex = 3;
			((ButtonBase)RB_Inv_amt).Text = "Invoice Amount";
			((ButtonBase)RB_Inv_amt).UseVisualStyleBackColor = true;
			((ButtonBase)RB_Inv_no).AutoSize = true;
			RB_Inv_no.Checked = true;
			((Control)RB_Inv_no).ForeColor = Color.Red;
			RadioButton rB_Inv_no = RB_Inv_no;
			location = new Point(10, 50);
			((Control)rB_Inv_no).Location = location;
			((Control)RB_Inv_no).Name = "RB_Inv_no";
			RadioButton rB_Inv_no2 = RB_Inv_no;
			size = new Size(125, 19);
			((Control)rB_Inv_no2).Size = size;
			((Control)RB_Inv_no).TabIndex = 2;
			RB_Inv_no.TabStop = true;
			((ButtonBase)RB_Inv_no).Text = "Invoice Number";
			((ButtonBase)RB_Inv_no).UseVisualStyleBackColor = true;
			SizeF autoScaleDimensions = new SizeF(6f, 13f);
			((ContainerControl)this).AutoScaleDimensions = autoScaleDimensions;
			((ContainerControl)this).AutoScaleMode = (AutoScaleMode)1;
			size = new Size(1279, 543);
			((Form)this).ClientSize = size;
			((Control)this).Controls.Add((Control)(object)DataGridView1);
			((Control)this).Controls.Add((Control)(object)grid1);
			((Control)this).Controls.Add((Control)(object)grid2);
			((Control)this).Controls.Add((Control)(object)DataGridView2);
			((Control)this).Controls.Add((Control)(object)GroupBox1);
			((Control)this).Controls.Add((Control)(object)Label2);
			((Control)this).Controls.Add((Control)(object)txtIgnoreDays);
			((Control)this).Controls.Add((Control)(object)Button6);
			((Control)this).Controls.Add((Control)(object)grid3);
			((Control)this).Controls.Add((Control)(object)Button7);
			((Control)this).Controls.Add((Control)(object)PB3);
			((Control)this).Controls.Add((Control)(object)Button5);
			((Control)this).Controls.Add((Control)(object)lblResultfilepath);
			((Control)this).Controls.Add((Control)(object)lblGSTRpath);
			((Control)this).Controls.Add((Control)(object)lblBahikhatapath);
			((Control)this).Controls.Add((Control)(object)Button4);
			((Control)this).Controls.Add((Control)(object)Label1);
			((Control)this).Controls.Add((Control)(object)TxtInfo);
			((Control)this).Controls.Add((Control)(object)DataGridView3);
			((Control)this).Controls.Add((Control)(object)Button3);
			((Control)this).Controls.Add((Control)(object)Button1);
			((Control)this).Controls.Add((Control)(object)Button2);
			((Control)this).Controls.Add((Control)(object)PB2);
			((Control)this).Controls.Add((Control)(object)PB1);
			((Form)this).Icon = (Icon)componentResourceManager.GetObject("$this.Icon");
			((Control)this).Name = "Form1";
			((Form)this).Text = "Matching GSTR-2 and GSTR-2A";
			((Form)this).WindowState = (FormWindowState)2;
			((ISupportInitialize)DataGridView1).EndInit();
			((ISupportInitialize)DataGridView2).EndInit();
			((ISupportInitialize)DataGridView3).EndInit();
			((ISupportInitialize)grid1).EndInit();
			((ISupportInitialize)grid2).EndInit();
			((ISupportInitialize)grid3).EndInit();
			((Control)GroupBox1).ResumeLayout(false);
			((Control)GroupBox1).PerformLayout();
			((Control)this).ResumeLayout(false);
			((Control)this).PerformLayout();
		}

		private void Button2_Click(object sender, EventArgs e)
		{
			//IL_000f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0016: Expected Obj, but got Unknown
			//IL_02d3: Unknown result type (might be due to invalid IL or missing references)
			//IL_02d9: Expected Obj, but got Unknown
			//IL_02ef: Unknown result type (might be due to invalid IL or missing references)
			//IL_02f5: Expected Obj, but got Unknown
			//IL_0371: Unknown result type (might be due to invalid IL or missing references)
			//IL_0377: Expected Obj, but got Unknown
			//IL_00e8: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ee: Invalid comparison between Unknown and I4
			PB1.Maximum = 25;
			OpenFileDialog val = new OpenFileDialog();
			if (Operators.CompareString(path1, "", TextCompare: false) == 0)
			{
				((FileDialog)val).InitialDirectory = "C:\\";
				try
				{
					string path = Application.StartupPath + "\\test.txt";
					if (File.Exists(path))
					{
						path = Application.StartupPath + "\\test.txt";
						string text = ((ServerComputer)MyProject.Computer).FileSystem.ReadAllText(path);
						path1 = text;
						((FileDialog)val).InitialDirectory = path1;
					}
					else
					{
						((FileDialog)val).InitialDirectory = "C:\\";
					}
				}
				catch (Exception ex)
				{
					ProjectData.SetProjectError(ex);
					Exception ex2 = ex;
					ProjectData.ClearProjectError();
				}
			}
			else
			{
				((FileDialog)val).InitialDirectory = path1;
			}
			((FileDialog)val).Filter = "Excel files (*.xlsx;*.xls)|*.xlsx;*.xls;";
			((FileDialog)val).RestoreDirectory = true;
			if ((int)((CommonDialog)val).ShowDialog((IWin32Window)(object)this) != 1)
			{
				return;
			}
			((Control)this).Cursor = CreateCursor(cursor_file);
			DataGridView1.DataSource = null;
			DataGridView1.Rows.Clear();
			((Control)PB1).Visible = true;
			PB1.Value = 5;
			FileInfo fileInfo = new FileInfo(((FileDialog)val).FileName);
			string fileName = ((FileDialog)val).FileName;
			string text2 = fileName;
			path1 = Path.GetDirectoryName(text2);
			try
			{
				string path2 = Application.StartupPath + "\\test.txt";
				if (File.Exists(path2))
				{
					StreamWriter streamWriter = new StreamWriter(path2);
					streamWriter.Write(path1);
					streamWriter.Close();
				}
			}
			catch (Exception ex3)
			{
				ProjectData.SetProjectError(ex3);
				Exception ex4 = ex3;
				ProjectData.ClearProjectError();
			}
			FileInfo fileInfo2 = ((ServerComputer)MyProject.Computer).FileSystem.GetFileInfo(text2);
			Guid clsid = new Guid("00024500-0000-0000-C000-000000000046");
			Application application = (Application)Activator.CreateInstance(Type.GetTypeFromCLSID(clsid));
			Workbook workbook = application.Workbooks.Open(text2, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value);
			string text3 = Conversions.ToString(NewLateBinding.LateGet(workbook.Worksheets[1], null, "Name", new object[0], null, null, null));
			text3 += "$";
			workbook.Close(Missing.Value, Missing.Value, Missing.Value);
			application.Quit();
			releaseObject(application);
			releaseObject(workbook);
			string fullName = fileInfo.FullName;
			text3 = "b2b$";
			OleDbConnection val2 = new OleDbConnection("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=" + fullName + ";Extended Properties=Excel 12.0;");
			string text4 = "Select * From [" + text3 + "]";
			OleDbDataAdapter val3 = new OleDbDataAdapter(text4, val2);
			DataSet dataSet = new DataSet();
			((DbDataAdapter)(object)val3).Fill(dataSet, "[" + text3 + "]");
			DataGridView1.DataSource = dataSet;
			DataGridView1.DataMember = "[" + text3 + "]";
			text3 = "cdnr$";
			text4 = "Select * From [" + text3 + "]";
			text4 = "Select [Summary For CDNR(6C)],F2,FORMAT([F3],'dd/MM/yyyy') as F3,F4,FORMAT([F3],'dd/MM/yyyy') as F5,F6,F7,F8,F9,F10,F11,F12,F13,F14,F15,F16,F17,F18,F19,F20,F22,F23 From [" + text3 + "]";
			val3 = new OleDbDataAdapter(text4, val2);
			dataSet = new DataSet();
			((DbDataAdapter)(object)val3).Fill(dataSet, "[" + text3 + "]");
			grid1.DataSource = dataSet;
			grid1.DataMember = "[" + text3 + "]";
			int num = 6;
			checked
			{
				int num2;
				int num3;
				do
				{
					PB1.Value = (int)Math.Round(Conversion.Val(PB1.Value) + 1.0);
					Thread.Sleep(10);
					num++;
					num2 = num;
					num3 = 25;
				}
				while (num2 <= num3);
				((Control)PB1).Visible = false;
				lblBahikhatapath.Text = text2;
				((Control)lblBahikhatapath).ForeColor = Color.Green;
				((Control)this).Cursor = Cursors.Default;
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

		private void Button1_Click(object sender, EventArgs e)
		{
			//IL_001e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0025: Expected Obj, but got Unknown
			//IL_0163: Unknown result type (might be due to invalid IL or missing references)
			//IL_0169: Invalid comparison between Unknown and I4
			//IL_09c1: Unknown result type (might be due to invalid IL or missing references)
			//IL_09c7: Expected Obj, but got Unknown
			//IL_09f4: Unknown result type (might be due to invalid IL or missing references)
			//IL_09fa: Expected Obj, but got Unknown
			//IL_0b2d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0b33: Expected Obj, but got Unknown
			//IL_0cfe: Unknown result type (might be due to invalid IL or missing references)
			//IL_0d04: Expected Obj, but got Unknown
			int try0001_dispatch = -1;
			checked
			{
				int num3 = default;
				int num = default;
				int num2 = default;
				string text = default;
				string text2 = default;
				bool flag = default;
				string text3 = default;
				OpenFileDialog val = default;
				string text4 = default;
				string text5 = default;
				FileInfo fileInfo = default;
				string fileName = default;
				string path = default;
				StreamWriter streamWriter = default;
				Application application = default;
				Workbook workbook = default;
				bool flag2 = default;
				bool flag3 = default;
				DataSet dataSet = default;
				OleDbDataAdapter val2 = default;
				OleDbConnection val3 = default;
				int num5 = default;
				int count = default;
				int num6 = default;
				int count2 = default;
				int num10 = default;
				string fullName = default;
				while (true)
				{
					try
					{
						/*Note: ILSpy has introduced the following switch to emulate a goto from catch-block to try-block*/;
						FileInfo fileInfo2;
						Guid clsid;
						Type? typeFromHandle;
						object[] array;
						object[] array2;
						object[] arguments;
						object left;
						Type? typeFromHandle2;
						object[] array6;
						object[] arguments2;
						object left2;
						Type? typeFromHandle3;
						object[] array7;
						object[] arguments3;
						object left3;
						int num7;
						Type? typeFromHandle4;
						object[] array8;
						object[] arguments4;
						object left4;
						Type? typeFromHandle5;
						object[] array9;
						object[] arguments5;
						object left5;
						int num9;
						int num11;
						int num8;
						Type? typeFromHandle6;
						object[] array10;
						object[] arguments6;
						object left6;
						Type? typeFromHandle7;
						object[] array5;
						object[] array11;
						object instance;
						object[] array3;
						object[] arguments7;
						bool[] array4;
						object left7;
						switch (try0001_dispatch)
						{
						default:
							ProjectData.ClearProjectError();
							num3 = -2;
							goto IL_000a;
						case 4249:
							{
								num = num2;
								switch ((num3 <= -2) ? 1 : num3)
								{
								case 1:
									break;
								default:
									goto end_IL_0001;
								}
								int num4 = unchecked(num + 1);
								num = 0;
								switch (num4)
								{
								case 1:
									break;
								case 2:
									goto IL_000a;
								case 3:
									goto IL_001b;
								case 4:
									goto IL_0025;
								case 5:
									goto IL_0045;
								case 6:
									goto IL_0055;
								case 7:
									goto IL_0069;
								case 8:
									goto IL_0079;
								case 9:
									goto IL_008d;
								case 10:
									goto IL_00a4;
								case 11:
									goto IL_00b0;
								case 13:
									goto IL_00c4;
								case 14:
									goto IL_00c9;
								case 17:
									goto IL_00dd;
								case 18:
									goto IL_00e2;
								case 12:
								case 15:
								case 16:
								case 19:
								case 20:
									goto IL_00f5;
								case 21:
									goto IL_0113;
								case 23:
									goto IL_0126;
								case 24:
									goto IL_012b;
								case 22:
								case 25:
								case 26:
									goto IL_013e;
								case 27:
									goto IL_014f;
								case 28:
									goto IL_015c;
								case 29:
									goto IL_0174;
								case 30:
									goto IL_018a;
								case 31:
									goto IL_019b;
								case 32:
									goto IL_01ac;
								case 33:
									goto IL_01bd;
								case 34:
									goto IL_01d2;
								case 35:
									goto IL_01e3;
								case 36:
									goto IL_01f8;
								case 37:
									goto IL_020a;
								case 38:
									goto IL_0217;
								case 39:
									goto IL_021f;
								case 40:
									goto IL_0230;
								case 41:
									goto IL_0245;
								case 42:
									goto IL_0256;
								case 43:
									goto IL_0263;
								case 44:
									goto IL_0275;
								case 45:
								case 46:
									goto IL_0282;
								case 47:
									goto IL_0299;
								case 48:
									goto IL_02bd;
								case 49:
									goto IL_0317;
								case 50:
									goto IL_0348;
								case 51:
									goto IL_035a;
								case 52:
									goto IL_0361;
								case 53:
									goto IL_0368;
								case 54:
									goto IL_036f;
								case 55:
									goto IL_0421;
								case 56:
									goto IL_042c;
								case 58:
									goto IL_0438;
								case 59:
									goto IL_04ea;
								case 60:
									goto IL_04f5;
								case 62:
									goto IL_0501;
								case 63:
									goto IL_05b3;
								case 64:
									goto IL_05be;
								case 57:
								case 61:
								case 65:
								case 66:
									goto IL_05c6;
								case 67:
									goto IL_05e0;
								case 68:
									goto IL_0693;
								case 70:
								case 71:
									goto IL_06a9;
								case 72:
									goto IL_075c;
								case 74:
								case 75:
									goto IL_076f;
								case 69:
								case 73:
								case 76:
									goto IL_0787;
								case 77:
									goto IL_07a1;
								case 78:
									goto IL_0854;
								case 80:
								case 81:
									goto IL_086a;
								case 82:
									goto IL_091d;
								case 84:
								case 85:
									goto IL_0930;
								case 79:
								case 83:
								case 86:
									goto IL_0948;
								case 87:
									goto IL_0963;
								case 88:
									goto IL_096f;
								case 89:
									goto IL_097c;
								case 90:
									goto IL_0989;
								case 91:
									goto IL_0996;
								case 92:
									goto IL_09ac;
								case 93:
									goto IL_09c7;
								case 94:
									goto IL_09d6;
								case 95:
									goto IL_09ed;
								case 96:
									goto IL_09fa;
								case 97:
									goto IL_0a05;
								case 98:
									goto IL_0a23;
								case 99:
									goto IL_0a5d;
								case 101:
									goto IL_0a76;
								case 102:
									goto IL_0ab0;
								case 105:
									goto IL_0aca;
								case 106:
									goto IL_0ad6;
								case 107:
									goto IL_0ae9;
								case 109:
									goto IL_0b02;
								case 110:
									goto IL_0b0e;
								case 100:
								case 103:
								case 104:
								case 108:
								case 111:
								case 112:
									goto IL_0b26;
								case 113:
									goto IL_0b33;
								case 114:
									goto IL_0b3e;
								case 115:
									goto IL_0b5c;
								case 116:
									goto IL_0b6e;
								case 117:
									goto IL_0b8f;
								case 118:
									goto IL_0ba5;
								case 119:
									goto IL_0bbc;
								case 120:
									goto IL_0bcb;
								case 121:
									goto IL_0be2;
								case 122:
									goto IL_0c1c;
								case 124:
									goto IL_0c35;
								case 125:
									goto IL_0c6f;
								case 128:
									goto IL_0c89;
								case 129:
									goto IL_0c98;
								case 130:
									goto IL_0cae;
								case 132:
									goto IL_0cca;
								case 133:
									goto IL_0cd9;
								case 123:
								case 126:
								case 127:
								case 131:
								case 134:
								case 135:
									goto IL_0cf4;
								case 136:
									goto IL_0d04;
								case 137:
									goto IL_0d12;
								case 138:
									goto IL_0d33;
								case 139:
									goto IL_0d48;
								case 140:
									goto IL_0d6c;
								case 141:
									goto IL_0d76;
								case 142:
									goto IL_0dae;
								case 143:
									goto IL_0dbd;
								case 144:
									goto IL_0dd5;
								case 145:
									goto IL_0de9;
								case 146:
									goto IL_0dfe;
								case 147:
									goto end_IL_0001_2;
								default:
									goto end_IL_0001;
								case 148:
								case 149:
									goto end_IL_0001_3;
								}
								goto default;
							}
							IL_0cae:
							num2 = 130;
							text = "Select [GSTINUIN],[Party Name],[Invoice Number],[Invoice Date],[Note Type],[NoteRefund Number],[NoteRefund date],[Reason For Issuing document],[Place Of Supply],[Pre GST],[NoteRefund Value],[Taxable Value],[Rate],sum([IGST]) as [IGST1],sum([CGST]) as [CGST1],sum([SGST]) as [SGST1],sum([CESS Amount]) as [CESS Amount1],[Uploaded On],[Return Filed] from [" + text2 + "] Group by [GSTINUIN],[Party Name],[Invoice Number],[Invoice Date],[Note Type],[NoteRefund Number],[NoteRefund date],[Reason For Issuing document],[Place Of Supply],[Pre GST],[NoteRefund Value],[Taxable Value],[Rate],[Uploaded On],[Return Filed]";
							goto IL_0cf4;
							IL_0cca:
							num2 = 132;
							if (flag)
							{
								goto IL_0cd9;
							}
							goto IL_0cf4;
							IL_0c98:
							num2 = 129;
							Edit_Excel(text3, type_notes);
							goto IL_0cae;
							IL_0cd9:
							num2 = 133;
							text = "Select distinct * From [" + text2 + "] WHERE f8<>'-'";
							goto IL_0cf4;
							IL_000a:
							num2 = 2;
							PB2.Maximum = 25;
							goto IL_001b;
							IL_001b:
							num2 = 3;
							val = new OpenFileDialog();
							goto IL_0025;
							IL_0025:
							num2 = 4;
							if (Operators.CompareString(path1, "", TextCompare: false) == 0)
							{
								goto IL_0045;
							}
							goto IL_00dd;
							IL_0045:
							num2 = 5;
							((FileDialog)val).InitialDirectory = "C:\\";
							goto IL_0055;
							IL_0055:
							num2 = 6;
							text4 = Application.StartupPath + "\\test.txt";
							goto IL_0069;
							IL_0069:
							num2 = 7;
							if (File.Exists(text4))
							{
								goto IL_0079;
							}
							goto IL_00c4;
							IL_0079:
							num2 = 8;
							text4 = Application.StartupPath + "\\test.txt";
							goto IL_008d;
							IL_008d:
							num2 = 9;
							text5 = ((ServerComputer)MyProject.Computer).FileSystem.ReadAllText(text4);
							goto IL_00a4;
							IL_00a4:
							num2 = 10;
							path1 = text5;
							goto IL_00b0;
							IL_00b0:
							num2 = 11;
							((FileDialog)val).InitialDirectory = path1;
							goto IL_00f5;
							IL_00c4:
							num2 = 13;
							goto IL_00c9;
							IL_00c9:
							num2 = 14;
							((FileDialog)val).InitialDirectory = "C:\\";
							goto IL_00f5;
							IL_00dd:
							num2 = 17;
							goto IL_00e2;
							IL_00e2:
							num2 = 18;
							((FileDialog)val).InitialDirectory = path1;
							goto IL_00f5;
							IL_00f5:
							num2 = 20;
							if (Operators.CompareString(path1, "", TextCompare: false) == 0)
							{
								goto IL_0113;
							}
							goto IL_0126;
							IL_0113:
							num2 = 21;
							((FileDialog)val).InitialDirectory = "C:\\";
							goto IL_013e;
							IL_0126:
							num2 = 23;
							goto IL_012b;
							IL_012b:
							num2 = 24;
							((FileDialog)val).InitialDirectory = path1;
							goto IL_013e;
							IL_013e:
							num2 = 26;
							((FileDialog)val).Filter = "Excel files (*.xlsx;*.xls)|*.xlsx;*.xls;";
							goto IL_014f;
							IL_014f:
							num2 = 27;
							((FileDialog)val).RestoreDirectory = true;
							goto IL_015c;
							IL_015c:
							num2 = 28;
							if (unchecked((int)((CommonDialog)val).ShowDialog((IWin32Window)(object)this)) != 1)
							{
								goto end_IL_0001_3;
							}
							goto IL_0174;
							IL_0174:
							num2 = 29;
							((Control)this).Cursor = CreateCursor(cursor_file);
							goto IL_018a;
							IL_018a:
							num2 = 30;
							((Control)PB2).Visible = true;
							goto IL_019b;
							IL_019b:
							num2 = 31;
							PB2.Value = 5;
							goto IL_01ac;
							IL_01ac:
							num2 = 32;
							DataGridView2.DataSource = null;
							goto IL_01bd;
							IL_01bd:
							num2 = 33;
							DataGridView2.Rows.Clear();
							goto IL_01d2;
							IL_01d2:
							num2 = 34;
							grid2.DataSource = null;
							goto IL_01e3;
							IL_01e3:
							num2 = 35;
							grid2.Rows.Clear();
							goto IL_01f8;
							IL_01f8:
							num2 = 36;
							fileInfo = new FileInfo(((FileDialog)val).FileName);
							goto IL_020a;
							IL_020a:
							num2 = 37;
							fileName = ((FileDialog)val).FileName;
							goto IL_0217;
							IL_0217:
							num2 = 38;
							text3 = fileName;
							goto IL_021f;
							IL_021f:
							num2 = 39;
							path1 = Path.GetDirectoryName(text3);
							goto IL_0230;
							IL_0230:
							num2 = 40;
							path = Application.StartupPath + "\\test.txt";
							goto IL_0245;
							IL_0245:
							num2 = 41;
							if (File.Exists(path))
							{
								goto IL_0256;
							}
							goto IL_0282;
							IL_0256:
							num2 = 42;
							streamWriter = new StreamWriter(path);
							goto IL_0263;
							IL_0263:
							num2 = 43;
							streamWriter.Write(path1);
							goto IL_0275;
							IL_0275:
							num2 = 44;
							streamWriter.Close();
							goto IL_0282;
							IL_0282:
							num2 = 46;
							fileInfo2 = ((ServerComputer)MyProject.Computer).FileSystem.GetFileInfo(text3);
							goto IL_0299;
							IL_0299:
							num2 = 47;
							clsid = new Guid("00024500-0000-0000-C000-000000000046");
							application = (Application)Activator.CreateInstance(Type.GetTypeFromCLSID(clsid));
							goto IL_02bd;
							IL_02bd:
							num2 = 48;
							workbook = application.Workbooks.Open(text3, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value);
							goto IL_0317;
							IL_0317:
							num2 = 49;
							text2 = Conversions.ToString(NewLateBinding.LateGet(workbook.Worksheets[1], null, "Name", new object[0], null, null, null));
							goto IL_0348;
							IL_0348:
							num2 = 50;
							text2 += "$";
							goto IL_035a;
							IL_035a:
							num2 = 51;
							flag2 = false;
							goto IL_0361;
							IL_0361:
							num2 = 52;
							flag3 = false;
							goto IL_0368;
							IL_0368:
							num2 = 53;
							flag = false;
							goto IL_036f;
							IL_036f:
							num2 = 54;
							typeFromHandle = typeof(Strings);
							array = new object[1];
							array2 = array;
							instance = workbook.Worksheets[1];
							array2[0] = RuntimeHelpers.GetObjectValue(NewLateBinding.LateGet(instance, null, "Name", new object[0], null, null, null));
							array3 = array;
							arguments = array3;
							array4 = new bool[1] { true };
							left = NewLateBinding.LateGet(null, typeFromHandle, "UCase", arguments, null, null, array4);
							if (array4[0])
							{
								NewLateBinding.LateSetComplex(instance, null, "Name", new object[1] { RuntimeHelpers.GetObjectValue(array3[0]) }, null, null, OptimisticSet: true, RValueBase: true);
							}
							if (Operators.ConditionalCompareObjectEqual(left, Strings.UCase("Invoice"), TextCompare: false))
							{
								goto IL_0421;
							}
							goto IL_0438;
							IL_0d04:
							num2 = 136;
							dataSet = new DataSet();
							goto IL_0d12;
							IL_0cf4:
							num2 = 135;
							val2 = new OleDbDataAdapter(text, val3);
							goto IL_0d04;
							IL_0421:
							num2 = 55;
							first_type = true;
							goto IL_042c;
							IL_042c:
							num2 = 56;
							flag2 = true;
							goto IL_05c6;
							IL_0438:
							num2 = 58;
							typeFromHandle2 = typeof(Strings);
							array5 = new object[1];
							array6 = array5;
							instance = workbook.Worksheets[1];
							array6[0] = RuntimeHelpers.GetObjectValue(NewLateBinding.LateGet(instance, null, "Name", new object[0], null, null, null));
							array3 = array5;
							arguments2 = array3;
							array4 = new bool[1] { true };
							left2 = NewLateBinding.LateGet(null, typeFromHandle2, "UCase", arguments2, null, null, array4);
							if (array4[0])
							{
								NewLateBinding.LateSetComplex(instance, null, "Name", new object[1] { RuntimeHelpers.GetObjectValue(array3[0]) }, null, null, OptimisticSet: true, RValueBase: true);
							}
							if (Operators.ConditionalCompareObjectEqual(left2, Strings.UCase("Main"), TextCompare: false))
							{
								goto IL_04ea;
							}
							goto IL_0501;
							IL_0d33:
							num2 = 138;
							grid2.DataSource = dataSet;
							goto IL_0d48;
							IL_0d12:
							num2 = 137;
							((DbDataAdapter)(object)val2).Fill(dataSet, "[" + text2 + "]");
							goto IL_0d33;
							IL_04ea:
							num2 = 59;
							second_type = true;
							goto IL_04f5;
							IL_04f5:
							num2 = 60;
							flag3 = true;
							goto IL_05c6;
							IL_0501:
							num2 = 62;
							typeFromHandle3 = typeof(Strings);
							array5 = new object[1];
							array7 = array5;
							instance = workbook.Worksheets[1];
							array7[0] = RuntimeHelpers.GetObjectValue(NewLateBinding.LateGet(instance, null, "Name", new object[0], null, null, null));
							array3 = array5;
							arguments3 = array3;
							array4 = new bool[1] { true };
							left3 = NewLateBinding.LateGet(null, typeFromHandle3, "UCase", arguments3, null, null, array4);
							if (array4[0])
							{
								NewLateBinding.LateSetComplex(instance, null, "Name", new object[1] { RuntimeHelpers.GetObjectValue(array3[0]) }, null, null, OptimisticSet: true, RValueBase: true);
							}
							if (Operators.ConditionalCompareObjectEqual(left3, Strings.UCase("Read me"), TextCompare: false))
							{
								goto IL_05b3;
							}
							goto IL_05c6;
							IL_0d6c:
							num2 = 140;
							num5 = 6;
							goto IL_0d76;
							IL_0d48:
							num2 = 139;
							grid2.DataMember = "[" + text2 + "]";
							goto IL_0d6c;
							IL_05b3:
							num2 = 63;
							third_type = true;
							goto IL_05be;
							IL_05be:
							num2 = 64;
							flag = true;
							goto IL_05c6;
							IL_05c6:
							num2 = 66;
							count = workbook.Worksheets.Count;
							num6 = 1;
							goto IL_077a;
							IL_077a:
							num7 = num6;
							num8 = count;
							if (num7 <= num8)
							{
								goto IL_05e0;
							}
							goto IL_0787;
							IL_05e0:
							num2 = 67;
							typeFromHandle4 = typeof(Strings);
							array5 = new object[1];
							array8 = array5;
							instance = workbook.Worksheets[num6];
							array8[0] = RuntimeHelpers.GetObjectValue(NewLateBinding.LateGet(instance, null, "Name", new object[0], null, null, null));
							array3 = array5;
							arguments4 = array3;
							array4 = new bool[1] { true };
							left4 = NewLateBinding.LateGet(null, typeFromHandle4, "UCase", arguments4, null, null, array4);
							if (array4[0])
							{
								NewLateBinding.LateSetComplex(instance, null, "Name", new object[1] { RuntimeHelpers.GetObjectValue(array3[0]) }, null, null, OptimisticSet: true, RValueBase: true);
							}
							if (Operators.ConditionalCompareObjectEqual(left4, Strings.UCase("invoice"), TextCompare: false))
							{
								goto IL_0693;
							}
							goto IL_06a9;
							IL_0dae:
							num2 = 142;
							Thread.Sleep(10);
							goto IL_0dbd;
							IL_0d76:
							num2 = 141;
							PB2.Value = (int)Math.Round(Conversion.Val(PB2.Value) + 1.0);
							goto IL_0dae;
							IL_0693:
							num2 = 68;
							type = "invoice";
							goto IL_0787;
							IL_06a9:
							num2 = 71;
							typeFromHandle5 = typeof(Strings);
							array5 = new object[1];
							array9 = array5;
							instance = workbook.Worksheets[num6];
							array9[0] = RuntimeHelpers.GetObjectValue(NewLateBinding.LateGet(instance, null, "Name", new object[0], null, null, null));
							array3 = array5;
							arguments5 = array3;
							array4 = new bool[1] { true };
							left5 = NewLateBinding.LateGet(null, typeFromHandle5, "UCase", arguments5, null, null, array4);
							if (array4[0])
							{
								NewLateBinding.LateSetComplex(instance, null, "Name", new object[1] { RuntimeHelpers.GetObjectValue(array3[0]) }, null, null, OptimisticSet: true, RValueBase: true);
							}
							if (Operators.ConditionalCompareObjectEqual(left5, Strings.UCase("b2b"), TextCompare: false))
							{
								goto IL_075c;
							}
							goto IL_076f;
							IL_0dd5:
							num2 = 144;
							((Control)PB2).Visible = false;
							goto IL_0de9;
							IL_0dbd:
							num2 = 143;
							num5++;
							num9 = num5;
							num8 = 25;
							if (num9 <= num8)
							{
								goto IL_0d76;
							}
							goto IL_0dd5;
							IL_075c:
							num2 = 72;
							type = "b2b";
							goto IL_0787;
							IL_0787:
							num2 = 76;
							count2 = workbook.Worksheets.Count;
							num10 = 1;
							goto IL_093b;
							IL_093b:
							num11 = num10;
							num8 = count2;
							if (num11 <= num8)
							{
								goto IL_07a1;
							}
							goto IL_0948;
							IL_07a1:
							num2 = 77;
							typeFromHandle6 = typeof(Strings);
							array5 = new object[1];
							array10 = array5;
							instance = workbook.Worksheets[num10];
							array10[0] = RuntimeHelpers.GetObjectValue(NewLateBinding.LateGet(instance, null, "Name", new object[0], null, null, null));
							array3 = array5;
							arguments6 = array3;
							array4 = new bool[1] { true };
							left6 = NewLateBinding.LateGet(null, typeFromHandle6, "UCase", arguments6, null, null, array4);
							if (array4[0])
							{
								NewLateBinding.LateSetComplex(instance, null, "Name", new object[1] { RuntimeHelpers.GetObjectValue(array3[0]) }, null, null, OptimisticSet: true, RValueBase: true);
							}
							if (Operators.ConditionalCompareObjectEqual(left6, Strings.UCase("invoice"), TextCompare: false))
							{
								goto IL_0854;
							}
							goto IL_086a;
							IL_0dfe:
							num2 = 146;
							((Control)lblGSTRpath).ForeColor = Color.Green;
							break;
							IL_0de9:
							num2 = 145;
							lblGSTRpath.Text = text3;
							goto IL_0dfe;
							IL_0854:
							num2 = 78;
							type_notes = "note";
							goto IL_0948;
							IL_086a:
							num2 = 81;
							typeFromHandle7 = typeof(Strings);
							array5 = new object[1];
							array11 = array5;
							instance = workbook.Worksheets[num10];
							array11[0] = RuntimeHelpers.GetObjectValue(NewLateBinding.LateGet(instance, null, "Name", new object[0], null, null, null));
							array3 = array5;
							arguments7 = array3;
							array4 = new bool[1] { true };
							left7 = NewLateBinding.LateGet(null, typeFromHandle7, "UCase", arguments7, null, null, array4);
							if (array4[0])
							{
								NewLateBinding.LateSetComplex(instance, null, "Name", new object[1] { RuntimeHelpers.GetObjectValue(array3[0]) }, null, null, OptimisticSet: true, RValueBase: true);
							}
							if (Operators.ConditionalCompareObjectEqual(left7, Strings.UCase("b2b"), TextCompare: false))
							{
								goto IL_091d;
							}
							goto IL_0930;
							IL_076f:
							num2 = 75;
							num6++;
							goto IL_077a;
							IL_0930:
							num2 = 85;
							num10++;
							goto IL_093b;
							IL_091d:
							num2 = 82;
							type_notes = "cdnr";
							goto IL_0948;
							IL_0948:
							num2 = 86;
							workbook.Close(Missing.Value, Missing.Value, Missing.Value);
							goto IL_0963;
							IL_0963:
							num2 = 87;
							application.Quit();
							goto IL_096f;
							IL_096f:
							num2 = 88;
							releaseObject(application);
							goto IL_097c;
							IL_097c:
							num2 = 89;
							releaseObject(workbook);
							goto IL_0989;
							IL_0989:
							num2 = 90;
							fullName = fileInfo.FullName;
							goto IL_0996;
							IL_0996:
							num2 = 91;
							text2 = type + "$";
							goto IL_09ac;
							IL_09ac:
							num2 = 92;
							val3 = new OleDbConnection("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=" + fullName + ";Extended Properties=Excel 12.0;");
							goto IL_09c7;
							IL_09c7:
							num2 = 93;
							if (flag2)
							{
								goto IL_09d6;
							}
							goto IL_0aca;
							IL_09d6:
							num2 = 94;
							text = "Select * From [" + text2 + "]";
							goto IL_09ed;
							IL_09ed:
							num2 = 95;
							val2 = new OleDbDataAdapter(text, val3);
							goto IL_09fa;
							IL_09fa:
							num2 = 96;
							dataSet = new DataSet();
							goto IL_0a05;
							IL_0a05:
							num2 = 97;
							((DbDataAdapter)(object)val2).Fill(dataSet, "[" + text2 + "]");
							goto IL_0a23;
							IL_0a23:
							num2 = 98;
							if (Conversion.Val(dataSet.Tables[0].Columns.Count.ToString()) == 20.0)
							{
								goto IL_0a5d;
							}
							goto IL_0a76;
							IL_0a5d:
							num2 = 99;
							text = "Select F2 as F22,F2,F3,F4,F5,F6,F7,F8,F9,sum(F10) as F10,sum(F11) as F11,sum(F12) as F12,sum(F13) as F13,sum(F14) as F14,sum(F15) as F15,F16,F17,F18,F19,F20 from [" + text2 + "] Group by F2,F3,F4,F5,F6,F7,F8,F9,F16,F17,F18,F19,F20";
							goto IL_0b26;
							IL_0a76:
							num2 = 101;
							if (Conversion.Val(dataSet.Tables[0].Columns.Count.ToString()) == 19.0)
							{
								goto IL_0ab0;
							}
							goto IL_0b26;
							IL_0ab0:
							num2 = 102;
							text = "Select F2 as F22,F2,F3,F4,F5,F6,F7,F8,F9,sum(F10) as F10,sum(F11) as F11,sum(F12) as F12,sum(F13) as F13,sum(F14) as F14,sum(F15) as F15,F16,F17,F18,F19 from [" + text2 + "] Group by F2,F3,F4,F5,F6,F7,F8,F9,F16,F17,F18,F19";
							goto IL_0b26;
							IL_0aca:
							num2 = 105;
							if (flag3)
							{
								goto IL_0ad6;
							}
							goto IL_0b02;
							IL_0ad6:
							num2 = 106;
							Edit_Excel(text3, type);
							goto IL_0ae9;
							IL_0ae9:
							num2 = 107;
							text = "Select * From [" + text2 + "]";
							goto IL_0b26;
							IL_0b02:
							num2 = 109;
							if (flag)
							{
								goto IL_0b0e;
							}
							goto IL_0b26;
							IL_0b0e:
							num2 = 110;
							text = "Select F2 as F22,F2,F3,F4,F5,F6,F7,F8,F9,F10,sum(val(F11)) as F11,sum(F12) as F12,sum(F13) as F13,F14 from [" + text2 + "] WHERE f9<>'-' Group by F2,F3,F4,F5,F6,F7,F8,F9,F10,F14";
							goto IL_0b26;
							IL_0b26:
							num2 = 112;
							val2 = new OleDbDataAdapter(text, val3);
							goto IL_0b33;
							IL_0b33:
							num2 = 113;
							dataSet = new DataSet();
							goto IL_0b3e;
							IL_0b3e:
							num2 = 114;
							((DbDataAdapter)(object)val2).Fill(dataSet, "[" + text2 + "]");
							goto IL_0b5c;
							IL_0b5c:
							num2 = 115;
							DataGridView2.DataSource = dataSet;
							goto IL_0b6e;
							IL_0b6e:
							num2 = 116;
							DataGridView2.DataMember = "[" + text2 + "]";
							goto IL_0b8f;
							IL_0b8f:
							num2 = 117;
							text2 = type_notes + "$";
							goto IL_0ba5;
							IL_0ba5:
							num2 = 118;
							text = "Select * From [" + text2 + "]";
							goto IL_0bbc;
							IL_0bbc:
							num2 = 119;
							if (flag2)
							{
								goto IL_0bcb;
							}
							goto IL_0c89;
							IL_0bcb:
							num2 = 120;
							text = "Select * From [" + text2 + "]";
							goto IL_0be2;
							IL_0be2:
							num2 = 121;
							if (Conversion.Val(dataSet.Tables[0].Columns.Count.ToString()) == 20.0)
							{
								goto IL_0c1c;
							}
							goto IL_0c35;
							IL_0c1c:
							num2 = 122;
							text = "Select F2 as F22,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,F13,F14,sum(F15) as F15,sum(F16) as F16,sum(F17) as F17,sum(F18) as F18,F19,F20,F21,F22 from [" + text2 + "] Group by F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,F13,F14,F19,F20,F21,F22";
							goto IL_0cf4;
							IL_0c35:
							num2 = 124;
							if (Conversion.Val(dataSet.Tables[0].Columns.Count.ToString()) == 19.0)
							{
								goto IL_0c6f;
							}
							goto IL_0cf4;
							IL_0c6f:
							num2 = 125;
							text = "Select F2 as F22,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,F13,sum(val(F14)) as F14,sum(val(F15)) as F15,sum(val(F16)) as F16,sum(val(F17)) as F17,F18,F19,F20,F21 from [" + text2 + "] Group by F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,F13,F18,F19,F20,F21";
							goto IL_0cf4;
							IL_0c89:
							num2 = 128;
							if (flag3)
							{
								goto IL_0c98;
							}
							goto IL_0cca;
							end_IL_0001_2:
							break;
						}
						num2 = 147;
						((Control)this).Cursor = Cursors.Default;
						break;
						end_IL_0001:;
					}
					catch (Exception ex) when ((num3 != 0) & (num == 0))
					{
						ProjectData.SetProjectError(ex);
						try0001_dispatch = 4249;
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
		}

		private void Edit_Excel(string excelnm, string shtnm)
		{
			Guid clsid = new Guid("00024500-0000-0000-C000-000000000046");
			Application application = (Application)Activator.CreateInstance(Type.GetTypeFromCLSID(clsid));
			Workbook workbook = application.Workbooks.Open(excelnm, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value);
			Worksheet worksheet = (Worksheet)workbook.Worksheets[shtnm];
			application.DisplayAlerts = false;
			string text = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[1, 2], null, "value", new object[0], null, null, null));
			text = text.Replace("/", "");
			worksheet.Cells[1, 2] = text;
			text = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[1, 1], null, "value", new object[0], null, null, null));
			text = text.Replace("/", "");
			worksheet.Cells[1, 1] = text;
			text = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[1, 6], null, "value", new object[0], null, null, null));
			text = text.Replace("/", "");
			worksheet.Cells[1, 6] = text;
			text = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[1, 7], null, "value", new object[0], null, null, null));
			text = text.Replace("/", "");
			worksheet.Cells[1, 7] = text;
			text = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[1, 11], null, "value", new object[0], null, null, null));
			text = text.Replace("/", "");
			worksheet.Cells[1, 11] = text;
			text = Conversions.ToString(NewLateBinding.LateGet(worksheet.Cells[1, 4], null, "value", new object[0], null, null, null));
			text = text.Replace(".", "");
			worksheet.Cells[2, 4] = text;
			workbook.Save();
			application.DisplayAlerts = true;
			workbook.Close(Missing.Value, Missing.Value, Missing.Value);
			application.Quit();
		}

		private void Button3_Click(object sender, EventArgs e)
		{
			((Control)grid3).Visible = false;
			((Control)DataGridView3).Visible = true;
			((Control)DataGridView3).BringToFront();
			if (first_type)
			{
				Compare_invoice_First();
			}
			else if (second_type)
			{
				Compare_b2b_second();
			}
			else if (third_type)
			{
				Compare_Third();
			}
		}

		[DllImport("user32.dll", CharSet = CharSet.Unicode, EntryPoint = "LoadCursorFromFileW", ExactSpelling = true, SetLastError = true)]
		private static extern IntPtr LoadCursorFromFile([MarshalAs(UnmanagedType.VBByRefStr)] ref string filename);

		private static Cursor CreateCursor(string filename)
		{
			//IL_0030: Unknown result type (might be due to invalid IL or missing references)
			//IL_0036: Expected Obj, but got Unknown
			Cursor result = null;
			try
			{
				IntPtr intPtr = LoadCursorFromFile(ref filename);
				if (IntPtr.Zero.Equals((object?)(nint)intPtr))
				{
					throw new ApplicationException("Could not create cursor from file " + filename);
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

		private void Form1_Load(object sender, EventArgs e)
		{
			lblBahikhatapath.Text = "";
			lblGSTRpath.Text = "";
			lblResultfilepath.Text = "";
			if (Operators.CompareString(Module1.save_file_nm, "", TextCompare: false) == 0)
			{
				Module1.save_file_nm = "nwtestresult.xls";
			}
			cursor_file = Application.StartupPath + "\\wait.ICO";
			set_global_date_format();
		}

		public void set_global_date_format()
		{
			Thread.CurrentThread.CurrentCulture = new CultureInfo("en-IN", useUserOverride: true);
			Registry.SetValue("HKEY_CURRENT_USER\\Control Panel\\International", "sShortDate", "dd-MM-yyyy");
		}

		private void Compare_invoice_First()
		{
			//IL_00ef: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f9: Expected Obj, but got Unknown
			//IL_0e58: Unknown result type (might be due to invalid IL or missing references)
			//IL_0e62: Expected Obj, but got Unknown
			//IL_0eed: Unknown result type (might be due to invalid IL or missing references)
			//IL_0ef7: Expected Obj, but got Unknown
			//IL_1a9b: Unknown result type (might be due to invalid IL or missing references)
			//IL_1aa5: Expected Obj, but got Unknown
			//IL_1b30: Unknown result type (might be due to invalid IL or missing references)
			//IL_1b3a: Expected Obj, but got Unknown
			//IL_2869: Unknown result type (might be due to invalid IL or missing references)
			//IL_2873: Expected Obj, but got Unknown
			//IL_28d7: Unknown result type (might be due to invalid IL or missing references)
			//IL_28e1: Expected Obj, but got Unknown
			//IL_2945: Unknown result type (might be due to invalid IL or missing references)
			//IL_294f: Expected Obj, but got Unknown
			//IL_29b3: Unknown result type (might be due to invalid IL or missing references)
			//IL_29bd: Expected Obj, but got Unknown
			//IL_2a21: Unknown result type (might be due to invalid IL or missing references)
			//IL_2a2b: Expected Obj, but got Unknown
			//IL_2a91: Unknown result type (might be due to invalid IL or missing references)
			//IL_2a9b: Expected Obj, but got Unknown
			//IL_2b01: Unknown result type (might be due to invalid IL or missing references)
			//IL_2b0b: Expected Obj, but got Unknown
			((Control)this).Cursor = CreateCursor(cursor_file);
			((Control)PB3).Visible = true;
			PB3.Maximum = 100;
			long num = 0L;
			DataGridView3.ColumnCount = 13;
			DataGridView3.RowCount = 1;
			checked
			{
				DataGridView3.Rows[(int)num].Cells[0].Value = "Bills Found In Bahi-Khata But Not In GSTR-2A";
				DataGridView3.Columns[0].DefaultCellStyle.WrapMode = (DataGridViewTriState)1;
				DataGridView3.Rows[(int)num].Height = 50;
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				num++;
				DataGridView3.Columns[0].HeaderText = "";
				DataGridView3.Columns[0].HeaderText = "SR No.";
				DataGridView3.Columns[1].HeaderText = "Date";
				DataGridView3.Columns[2].HeaderText = "Party Name";
				DataGridView3.Columns[3].HeaderText = "GSTIN";
				DataGridView3.Columns[4].HeaderText = "Bill No.";
				DataGridView3.Columns[5].HeaderText = "Bill Amount";
				DataGridView3.Columns[6].HeaderText = "Taxable Amt";
				DataGridView3.Columns[7].HeaderText = "IGST";
				DataGridView3.Columns[8].HeaderText = "CGST";
				DataGridView3.Columns[9].HeaderText = "SGST";
				DataGridView3.Columns[10].HeaderText = "CESS";
				DataGridView3.Columns[11].HeaderText = "POS";
				DataGridView3.Columns[12].HeaderText = "Contact No";
				PB3.Value = 10;
				int num2 = DataGridView1.RowCount - 1;
				int num3 = 3;
				double num9 = default;
				double num10 = default;
				double num11 = default;
				double num12 = default;
				double num13 = default;
				double num14 = default;
				int num5;
				while (true)
				{
					int num4 = num3;
					num5 = num2;
					if (num4 > num5)
					{
						break;
					}
					try
					{
						if (Operators.CompareString(DataGridView1.Rows[num3].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
						{
							break;
						}
					}
					catch (Exception ex)
					{
						ProjectData.SetProjectError(ex);
						Exception ex2 = ex;
						if (Operators.CompareString(DataGridView1.Rows[num3].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					string text = DataGridView1.Rows[num3].Cells[0].Value.ToString();
					if (Operators.CompareString(text, "", TextCompare: false) == 0)
					{
						break;
					}
					string text2 = DataGridView1.Rows[num3].Cells[1].Value.ToString();
					string value = Strings.Format(Conversions.ToDate(DataGridView1.Rows[num3].Cells[2].Value.ToString()), "dd-MM-yyyy");
					string inputStr = DataGridView1.Rows[num3].Cells[8].Value.ToString();
					string inputStr2 = DataGridView1.Rows[num3].Cells[9].Value.ToString();
					string inputStr3 = DataGridView1.Rows[num3].Cells[10].Value.ToString();
					string inputStr4 = DataGridView1.Rows[num3].Cells[11].Value.ToString();
					string inputStr5 = DataGridView1.Rows[num3].Cells[12].Value.ToString();
					string text3 = DataGridView1.Rows[num3].Cells[3].Value.ToString();
					string text4 = DataGridView1.Rows[num3].Cells[4].Value.ToString();
					text4 = Strings.Mid(text4, 4, text4.Length);
					string value2 = DataGridView1.Rows[num3].Cells[18].Value.ToString();
					string value3 = DataGridView1.Rows[num3].Cells[19].Value.ToString();
					string input = text2;
					input = Regex.Replace(input, "[^\\w\\\\-]", "");
					input = input.Replace("-", "");
					bool flag = false;
					int num6 = DataGridView2.RowCount - 1;
					int num7 = 3;
					while (true)
					{
						int num8 = num7;
						num5 = num6;
						if (num8 > num5)
						{
							break;
						}
						try
						{
							if (Operators.ConditionalCompareObjectEqual(DataGridView2.Rows[num7].Cells[1].Value, null, TextCompare: false))
							{
								break;
							}
						}
						catch (Exception ex3)
						{
							ProjectData.SetProjectError(ex3);
							Exception ex4 = ex3;
							if (Operators.CompareString(DataGridView2.Rows[num7].Cells[1].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						if (!Information.IsDate(DataGridView2.Rows[num7].Cells[8].Value.ToString()))
						{
							break;
						}
						string right = DataGridView2.Rows[num7].Cells[2].Value.ToString();
						string text5 = DataGridView2.Rows[num7].Cells[5].Value.ToString();
						string value4 = Strings.Format(Conversions.ToDate(DataGridView2.Rows[num7].Cells[8].Value.ToString()), "dd-MM-yyyy");
						string text6 = DataGridView2.Rows[num7].Cells[10].Value.ToString();
						string text7 = DataGridView2.Rows[num7].Cells[12].Value.ToString();
						string text8 = DataGridView2.Rows[num7].Cells[13].Value.ToString();
						string text9 = DataGridView2.Rows[num7].Cells[14].Value.ToString();
						string text10 = DataGridView2.Rows[num7].Cells[15].Value.ToString();
						string text11 = DataGridView2.Rows[num7].Cells[1].Value.ToString();
						string right2 = DataGridView2.Rows[num7].Cells[9].Value.ToString();
						string input2 = text5;
						input2 = Regex.Replace(input2, "[^\\w\\\\-]", "");
						input2 = input2.Replace("-", "");
						string inputStr6 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value4), Conversions.ToDate(value)));
						flag = false;
						if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr6) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input, input2))
								{
									if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr6) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag = true;
										break;
									}
									flag = false;
								}
								else
								{
									flag = false;
								}
							}
							else
							{
								if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr6) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(text3, right2, TextCompare: false) == 0))
								{
									flag = true;
									break;
								}
								flag = false;
							}
						}
						num7++;
					}
					if (!flag)
					{
						DataGridView3.RowCount += 1;
						DataGridView3.Rows[(int)num].Cells[0].Value = num;
						DataGridView3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value), "dd-MM-yyyy");
						DataGridView3.Rows[(int)num].Cells[2].Value = value2;
						DataGridView3.Rows[(int)num].Cells[3].Value = text;
						DataGridView3.Rows[(int)num].Cells[4].Value = text2;
						DataGridView3.Rows[(int)num].Cells[5].Value = Strings.Format(Conversion.Val(text3), "0.00");
						DataGridView3.Rows[(int)num].Cells[6].Value = Strings.Format(Conversion.Val(inputStr), "0.00");
						DataGridView3.Rows[(int)num].Cells[7].Value = Strings.Format(Conversion.Val(inputStr2), "0.00");
						DataGridView3.Rows[(int)num].Cells[8].Value = Strings.Format(Conversion.Val(inputStr3), "0.00");
						DataGridView3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr4), "0.00");
						DataGridView3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr5), "0.00");
						DataGridView3.Rows[(int)num].Cells[11].Value = Strings.UCase(text4);
						DataGridView3.Rows[(int)num].Cells[12].Value = Strings.UCase(value3);
						num9 = Conversion.Val(num9) + Conversion.Val(text3);
						num10 = Conversion.Val(num10) + Conversion.Val(inputStr);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr2);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr3);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr4);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr5);
						num++;
					}
					num3++;
				}
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[3].Value = "Total : ";
				DataGridView3.Rows[(int)num].Cells[5].Value = num9;
				DataGridView3.Rows[(int)num].Cells[6].Value = num10;
				DataGridView3.Rows[(int)num].Cells[7].Value = num11;
				DataGridView3.Rows[(int)num].Cells[8].Value = num12;
				DataGridView3.Rows[(int)num].Cells[9].Value = num13;
				DataGridView3.Rows[(int)num].Cells[10].Value = num14;
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[0].Value = "Bills Found In GSTR-A But Not In Bahi-Khata";
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				DataGridView3.Rows[(int)num].Height = 50;
				num9 = 0.0;
				num10 = 0.0;
				num11 = 0.0;
				num12 = 0.0;
				num13 = 0.0;
				num14 = 0.0;
				num++;
				PB3.Value = 45;
				int num15 = DataGridView2.RowCount - 1;
				int num16 = 3;
				long num21 = default;
				while (true)
				{
					int num17 = num16;
					num5 = num15;
					if (num17 > num5)
					{
						break;
					}
					try
					{
						if (Operators.ConditionalCompareObjectEqual(DataGridView2.Rows[num16].Cells[2].Value, null, TextCompare: false))
						{
							break;
						}
					}
					catch (Exception ex5)
					{
						ProjectData.SetProjectError(ex5);
						Exception ex6 = ex5;
						if (Operators.CompareString(DataGridView2.Rows[num16].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					if (!Information.IsDate(DataGridView2.Rows[num16].Cells[8].Value.ToString()))
					{
						break;
					}
					string text12 = DataGridView2.Rows[num16].Cells[2].Value.ToString();
					string text13 = DataGridView2.Rows[num16].Cells[5].Value.ToString();
					string value5 = Strings.Format(Conversions.ToDate(DataGridView2.Rows[num16].Cells[8].Value.ToString()), "dd-MM-yyyy");
					string inputStr7 = DataGridView2.Rows[num16].Cells[10].Value.ToString();
					string inputStr8 = DataGridView2.Rows[num16].Cells[12].Value.ToString();
					string inputStr9 = DataGridView2.Rows[num16].Cells[13].Value.ToString();
					string inputStr10 = DataGridView2.Rows[num16].Cells[14].Value.ToString();
					string inputStr11 = DataGridView2.Rows[num16].Cells[15].Value.ToString();
					string text14 = DataGridView2.Rows[num16].Cells[9].Value.ToString();
					string value6 = DataGridView2.Rows[num16].Cells[1].Value.ToString();
					string value7 = DataGridView2.Rows[num16].Cells[7].Value.ToString();
					string input3 = text13;
					input3 = Regex.Replace(input3, "[^\\w\\\\-]", "");
					input3 = input3.Replace("-", "");
					bool flag2 = false;
					int num18 = DataGridView1.RowCount - 1;
					int num19 = 3;
					while (true)
					{
						int num20 = num19;
						num5 = num18;
						if (num20 > num5)
						{
							break;
						}
						try
						{
							if (Operators.CompareString(DataGridView1.Rows[num19].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
							{
								break;
							}
						}
						catch (Exception ex7)
						{
							ProjectData.SetProjectError(ex7);
							Exception ex8 = ex7;
							if (Operators.CompareString(DataGridView1.Rows[num19].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						string left = DataGridView1.Rows[num19].Cells[0].Value.ToString();
						if (Operators.CompareString(left, "", TextCompare: false) == 0)
						{
							break;
						}
						string text15 = DataGridView1.Rows[num19].Cells[1].Value.ToString();
						string value8 = Strings.Format(Conversions.ToDate(DataGridView1.Rows[num19].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string text16 = DataGridView1.Rows[num19].Cells[8].Value.ToString();
						string text17 = DataGridView1.Rows[num19].Cells[9].Value.ToString();
						string text18 = DataGridView1.Rows[num19].Cells[10].Value.ToString();
						string text19 = DataGridView1.Rows[num19].Cells[11].Value.ToString();
						string text20 = DataGridView1.Rows[num19].Cells[12].Value.ToString();
						string left2 = DataGridView1.Rows[num19].Cells[3].Value.ToString();
						string input4 = text15;
						input4 = Regex.Replace(input4, "[^\\w\\\\-]", "");
						input4 = input4.Replace("-", "");
						string inputStr12 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value5), Conversions.ToDate(value8)));
						flag2 = false;
						flag2 = false;
						if ((Operators.CompareString(left, text12, TextCompare: false) == 0) & (Conversion.Val(inputStr12) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input4, input3))
								{
									if ((Operators.CompareString(left, text12, TextCompare: false) == 0) & (Conversion.Val(inputStr12) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag2 = true;
										break;
									}
									flag2 = false;
								}
								else
								{
									flag2 = false;
								}
							}
							else
							{
								if ((Operators.CompareString(left, text12, TextCompare: false) == 0) & (Conversion.Val(inputStr12) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(left2, text14, TextCompare: false) == 0))
								{
									flag2 = true;
									break;
								}
								flag2 = false;
							}
						}
						num19++;
					}
					if (!flag2)
					{
						DataGridView3.RowCount += 1;
						DataGridView3.Rows[(int)num].Cells[0].Value = num21 + 1;
						DataGridView3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value5), "dd-MM-yyyy");
						DataGridView3.Rows[(int)num].Cells[2].Value = value6;
						DataGridView3.Rows[(int)num].Cells[3].Value = text12;
						DataGridView3.Rows[(int)num].Cells[4].Value = text13;
						DataGridView3.Rows[(int)num].Cells[5].Value = Strings.Format(Conversion.Val(text14), "0.00");
						DataGridView3.Rows[(int)num].Cells[6].Value = Strings.Format(Conversion.Val(inputStr7), "0.00");
						DataGridView3.Rows[(int)num].Cells[7].Value = Strings.Format(Conversion.Val(inputStr8), "0.00");
						DataGridView3.Rows[(int)num].Cells[8].Value = Strings.Format(Conversion.Val(inputStr9), "0.00");
						DataGridView3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr10), "0.00");
						DataGridView3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr11), "0.00");
						DataGridView3.Rows[(int)num].Cells[11].Value = value7;
						num9 = Conversion.Val(num9) + Conversion.Val(text14);
						num10 = Conversion.Val(num10) + Conversion.Val(inputStr7);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr8);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr9);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr10);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr11);
						num++;
						num21++;
					}
					num16++;
				}
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[3].Value = "Total : ";
				DataGridView3.Rows[(int)num].Cells[5].Value = num9;
				DataGridView3.Rows[(int)num].Cells[6].Value = num10;
				DataGridView3.Rows[(int)num].Cells[7].Value = num11;
				DataGridView3.Rows[(int)num].Cells[8].Value = num12;
				DataGridView3.Rows[(int)num].Cells[9].Value = num13;
				DataGridView3.Rows[(int)num].Cells[10].Value = num14;
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[0].Value = "Bills Found But Mismatch In Figures";
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				DataGridView3.Rows[(int)num].Height = 50;
				num++;
				PB3.Value = 75;
				int num22 = DataGridView2.RowCount - 1;
				int num23 = 3;
				long num28 = default;
				while (true)
				{
					int num24 = num23;
					num5 = num22;
					if (num24 > num5)
					{
						break;
					}
					try
					{
						if (Operators.CompareString(DataGridView2.Rows[num23].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
						{
							break;
						}
					}
					catch (Exception ex9)
					{
						ProjectData.SetProjectError(ex9);
						Exception ex10 = ex9;
						if (Operators.CompareString(DataGridView2.Rows[num23].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					if (!Information.IsDate(DataGridView2.Rows[num23].Cells[8].Value.ToString()))
					{
						break;
					}
					string text21 = DataGridView2.Rows[num23].Cells[2].Value.ToString();
					string text22 = DataGridView2.Rows[num23].Cells[5].Value.ToString();
					string value9 = Strings.Format(Conversions.ToDate(DataGridView2.Rows[num23].Cells[8].Value.ToString()), "dd-MM-yyyy");
					string inputStr13 = DataGridView2.Rows[num23].Cells[10].Value.ToString();
					string inputStr14 = DataGridView2.Rows[num23].Cells[12].Value.ToString();
					string inputStr15 = DataGridView2.Rows[num23].Cells[13].Value.ToString();
					string inputStr16 = DataGridView2.Rows[num23].Cells[14].Value.ToString();
					string inputStr17 = DataGridView2.Rows[num23].Cells[15].Value.ToString();
					string value10 = DataGridView2.Rows[num23].Cells[1].Value.ToString();
					string text23 = DataGridView2.Rows[num23].Cells[9].Value.ToString();
					string value11 = DataGridView2.Rows[num23].Cells[7].Value.ToString();
					string input5 = text22;
					input5 = Regex.Replace(input5, "[^\\w\\\\-]", "");
					input5 = input5.Replace("-", "");
					bool flag3 = false;
					bool flag4 = false;
					bool flag5 = false;
					bool flag6 = false;
					bool flag7 = false;
					bool flag8 = false;
					bool flag9 = false;
					bool flag10 = false;
					int num25 = DataGridView1.RowCount - 1;
					int num26 = 4;
					while (true)
					{
						int num27 = num26;
						num5 = num25;
						if (num27 > num5)
						{
							break;
						}
						try
						{
							if (Operators.CompareString(DataGridView1.Rows[num26].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
							{
								break;
							}
						}
						catch (Exception ex11)
						{
							ProjectData.SetProjectError(ex11);
							Exception ex12 = ex11;
							if (Operators.CompareString(DataGridView1.Rows[num26].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						if (!Information.IsDate(DataGridView1.Rows[num26].Cells[2].Value.ToString()))
						{
							break;
						}
						string left3 = DataGridView1.Rows[num26].Cells[0].Value.ToString();
						if (Operators.CompareString(left3, "", TextCompare: false) == 0)
						{
							break;
						}
						string text24 = DataGridView1.Rows[num26].Cells[1].Value.ToString();
						string value12 = Strings.Format(Conversions.ToDate(DataGridView1.Rows[num26].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string inputStr18 = DataGridView1.Rows[num26].Cells[8].Value.ToString();
						string inputStr19 = DataGridView1.Rows[num26].Cells[9].Value.ToString();
						string inputStr20 = DataGridView1.Rows[num26].Cells[10].Value.ToString();
						string inputStr21 = DataGridView1.Rows[num26].Cells[11].Value.ToString();
						string inputStr22 = DataGridView1.Rows[num26].Cells[12].Value.ToString();
						string text25 = DataGridView1.Rows[num26].Cells[3].Value.ToString();
						string text26 = DataGridView1.Rows[num26].Cells[4].Value.ToString();
						text26 = Strings.Mid(text26, 4, text26.Length);
						string input6 = text24;
						input6 = Regex.Replace(input6, "[^\\w\\\\-]", "");
						input6 = input6.Replace("-", "");
						string inputStr23 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value9), Conversions.ToDate(value12)));
						if ((Operators.CompareString(left3, text21, TextCompare: false) == 0) & (Conversion.Val(inputStr23) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							flag10 = false;
							bool flag11 = false;
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input6, input5))
								{
									if (Operators.CompareString(Strings.UCase(text26), Strings.UCase(value11), TextCompare: false) != 0)
									{
										flag10 = true;
										flag3 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(text25) - Conversion.Val(text23))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag4 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr18) - Conversion.Val(inputStr13))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag5 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr19) - Conversion.Val(inputStr14))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag6 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr20) - Conversion.Val(inputStr15))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag8 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr21) - Conversion.Val(inputStr16))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag7 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr22) - Conversion.Val(inputStr17))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag9 = true;
									}
									break;
								}
								flag10 = false;
							}
							else
							{
								if ((Operators.CompareString(left3, text21, TextCompare: false) == 0) & (Conversion.Val(inputStr23) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(text25, text23, TextCompare: false) == 0))
								{
									if (Operators.CompareString(Strings.UCase(text26), Strings.UCase(value11), TextCompare: false) != 0)
									{
										flag10 = true;
										flag3 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(text25) - Conversion.Val(text23))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag4 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr18) - Conversion.Val(inputStr13))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag5 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr19) - Conversion.Val(inputStr14))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag6 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr20) - Conversion.Val(inputStr15))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag8 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr21) - Conversion.Val(inputStr16))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag7 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr22) - Conversion.Val(inputStr17))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag9 = true;
									}
									break;
								}
								flag10 = false;
							}
						}
						num26++;
					}
					if (flag10)
					{
						DataGridView3.RowCount += 1;
						DataGridView3.Rows[(int)num].Cells[0].Value = num28 + 1;
						DataGridView3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value9), "dd-MM-yyyy");
						DataGridView3.Rows[(int)num].Cells[2].Value = value10;
						DataGridView3.Rows[(int)num].Cells[3].Value = text21;
						DataGridView3.Rows[(int)num].Cells[4].Value = text22;
						DataGridView3.Rows[(int)num].Cells[5].Value = Strings.Format(Conversion.Val(text23), "0.00");
						DataGridView3.Rows[(int)num].Cells[6].Value = Strings.Format(Conversion.Val(inputStr13), "0.00");
						DataGridView3.Rows[(int)num].Cells[7].Value = Strings.Format(Conversion.Val(inputStr14), "0.00");
						DataGridView3.Rows[(int)num].Cells[8].Value = Strings.Format(Conversion.Val(inputStr15), "0.00");
						DataGridView3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr16), "0.00");
						DataGridView3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr17), "0.00");
						DataGridView3.Rows[(int)num].Cells[11].Value = value11;
						if (flag3)
						{
							DataGridView3.Rows[(int)num].Cells[11].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[11].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag4)
						{
							DataGridView3.Rows[(int)num].Cells[5].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[5].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag5)
						{
							DataGridView3.Rows[(int)num].Cells[6].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[6].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag6)
						{
							DataGridView3.Rows[(int)num].Cells[7].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[7].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag7)
						{
							DataGridView3.Rows[(int)num].Cells[8].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[8].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag8)
						{
							DataGridView3.Rows[(int)num].Cells[9].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[9].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag9)
						{
							DataGridView3.Rows[(int)num].Cells[10].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[10].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						num++;
						num28++;
					}
					num23++;
				}
				DataGridView3.Columns[0].Width = 200;
				int num29 = 1;
				int num30;
				do
				{
					DataGridView3.Columns[num29].AutoSizeMode = (DataGridViewAutoSizeColumnMode)10;
					num29++;
					num30 = num29;
					num5 = 11;
				}
				while (num30 <= num5);
				int num31 = 5;
				int num32;
				do
				{
					DataGridView3.Columns[num31].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)64;
					num31++;
					num32 = num31;
					num5 = 10;
				}
				while (num32 <= num5);
				int num33 = 76;
				int num34;
				do
				{
					PB3.Value = num33;
					Thread.Sleep(10);
					num33++;
					num34 = num33;
					num5 = 100;
				}
				while (num34 <= num5);
				((Control)PB3).Visible = false;
				((Control)this).Cursor = Cursors.Default;
			}
		}

		private void Compare_b2b_second()
		{
			//IL_00ef: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f9: Expected Obj, but got Unknown
			//IL_0e35: Unknown result type (might be due to invalid IL or missing references)
			//IL_0e3f: Expected Obj, but got Unknown
			//IL_0eca: Unknown result type (might be due to invalid IL or missing references)
			//IL_0ed4: Expected Obj, but got Unknown
			//IL_1a30: Unknown result type (might be due to invalid IL or missing references)
			//IL_1a3a: Expected Obj, but got Unknown
			//IL_1ac5: Unknown result type (might be due to invalid IL or missing references)
			//IL_1acf: Expected Obj, but got Unknown
			//IL_2752: Unknown result type (might be due to invalid IL or missing references)
			//IL_275c: Expected Obj, but got Unknown
			//IL_27c0: Unknown result type (might be due to invalid IL or missing references)
			//IL_27ca: Expected Obj, but got Unknown
			//IL_282e: Unknown result type (might be due to invalid IL or missing references)
			//IL_2838: Expected Obj, but got Unknown
			//IL_289c: Unknown result type (might be due to invalid IL or missing references)
			//IL_28a6: Expected Obj, but got Unknown
			//IL_290a: Unknown result type (might be due to invalid IL or missing references)
			//IL_2914: Expected Obj, but got Unknown
			//IL_297a: Unknown result type (might be due to invalid IL or missing references)
			//IL_2984: Expected Obj, but got Unknown
			//IL_29ea: Unknown result type (might be due to invalid IL or missing references)
			//IL_29f4: Expected Obj, but got Unknown
			((Control)this).Cursor = CreateCursor(cursor_file);
			((Control)PB3).Visible = true;
			PB3.Maximum = 100;
			long num = 0L;
			DataGridView3.ColumnCount = 13;
			DataGridView3.RowCount = 1;
			checked
			{
				DataGridView3.Rows[(int)num].Cells[0].Value = "Bills Found In Bahi-Khata But Not In GSTR-2A";
				DataGridView3.Columns[0].DefaultCellStyle.WrapMode = (DataGridViewTriState)1;
				DataGridView3.Rows[(int)num].Height = 50;
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				num++;
				DataGridView3.Columns[0].HeaderText = "SR No.";
				DataGridView3.Columns[1].HeaderText = "Date";
				DataGridView3.Columns[2].HeaderText = "Party Name";
				DataGridView3.Columns[3].HeaderText = "GSTIN";
				DataGridView3.Columns[4].HeaderText = "Bill No.";
				DataGridView3.Columns[5].HeaderText = "Bill Amount";
				DataGridView3.Columns[6].HeaderText = "Taxable Amt";
				DataGridView3.Columns[7].HeaderText = "IGST";
				DataGridView3.Columns[8].HeaderText = "CGST";
				DataGridView3.Columns[9].HeaderText = "SGST";
				DataGridView3.Columns[10].HeaderText = "CESS";
				DataGridView3.Columns[11].HeaderText = "POS";
				DataGridView3.Columns[12].HeaderText = "Contact";
				PB3.Value = 10;
				int num2 = DataGridView1.RowCount - 1;
				int num3 = 3;
				double num9 = default;
				double num10 = default;
				double num11 = default;
				double num12 = default;
				double num13 = default;
				double num14 = default;
				int num5;
				while (true)
				{
					int num4 = num3;
					num5 = num2;
					if (num4 > num5)
					{
						break;
					}
					try
					{
						if (Operators.CompareString(DataGridView1.Rows[num3].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
						{
							break;
						}
					}
					catch (Exception ex)
					{
						ProjectData.SetProjectError(ex);
						Exception ex2 = ex;
						if (Operators.CompareString(DataGridView1.Rows[num3].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					string text = DataGridView1.Rows[num3].Cells[0].Value.ToString();
					if (Operators.CompareString(text, "", TextCompare: false) == 0)
					{
						break;
					}
					string text2 = DataGridView1.Rows[num3].Cells[1].Value.ToString();
					string value = Strings.Format(Conversions.ToDate(DataGridView1.Rows[num3].Cells[2].Value.ToString()), "dd-MM-yyyy");
					string inputStr = DataGridView1.Rows[num3].Cells[8].Value.ToString();
					string inputStr2 = DataGridView1.Rows[num3].Cells[9].Value.ToString();
					string inputStr3 = DataGridView1.Rows[num3].Cells[10].Value.ToString();
					string inputStr4 = DataGridView1.Rows[num3].Cells[11].Value.ToString();
					string inputStr5 = DataGridView1.Rows[num3].Cells[12].Value.ToString();
					string text3 = DataGridView1.Rows[num3].Cells[3].Value.ToString();
					string text4 = DataGridView1.Rows[num3].Cells[4].Value.ToString();
					text4 = Strings.Mid(text4, 4, text4.Length);
					string value2 = DataGridView1.Rows[num3].Cells[16].Value.ToString();
					string value3 = DataGridView1.Rows[num3].Cells[17].Value.ToString();
					string input = text2;
					input = Regex.Replace(input, "[^\\w\\\\-]", "");
					input = input.Replace("-", "");
					bool flag = false;
					int num6 = DataGridView2.RowCount - 1;
					int num7 = 0;
					while (true)
					{
						int num8 = num7;
						num5 = num6;
						if (num8 > num5)
						{
							break;
						}
						try
						{
							if (Operators.ConditionalCompareObjectEqual(DataGridView2.Rows[num7].Cells[1].Value, null, TextCompare: false))
							{
								break;
							}
						}
						catch (Exception ex3)
						{
							ProjectData.SetProjectError(ex3);
							Exception ex4 = ex3;
							if (Operators.CompareString(DataGridView2.Rows[num7].Cells[1].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						if (!Information.IsDate(DataGridView2.Rows[num7].Cells[4].Value.ToString()))
						{
							break;
						}
						string right = DataGridView2.Rows[num7].Cells[1].Value.ToString();
						string text5 = DataGridView2.Rows[num7].Cells[3].Value.ToString();
						string value4 = Strings.Format(Conversions.ToDate(DataGridView2.Rows[num7].Cells[4].Value.ToString()), "dd-MM-yyyy");
						string text6 = DataGridView2.Rows[num7].Cells[8].Value.ToString();
						string text7 = DataGridView2.Rows[num7].Cells[10].Value.ToString();
						string text8 = DataGridView2.Rows[num7].Cells[11].Value.ToString();
						string text9 = DataGridView2.Rows[num7].Cells[12].Value.ToString();
						string text10 = DataGridView2.Rows[num7].Cells[13].Value.ToString();
						string text11 = DataGridView2.Rows[num7].Cells[2].Value.ToString();
						string right2 = DataGridView2.Rows[num7].Cells[7].Value.ToString();
						string input2 = text5;
						input2 = Regex.Replace(input2, "[^\\w\\\\-]", "");
						input2 = input2.Replace("-", "");
						string inputStr6 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value4), Conversions.ToDate(value)));
						flag = false;
						if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr6) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input, input2))
								{
									if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr6) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag = true;
										break;
									}
									flag = false;
								}
								else
								{
									flag = false;
								}
							}
							else
							{
								if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr6) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(text3, right2, TextCompare: false) == 0))
								{
									flag = true;
									break;
								}
								flag = false;
							}
						}
						num7++;
					}
					if (!flag)
					{
						DataGridView3.RowCount += 1;
						DataGridView3.Rows[(int)num].Cells[0].Value = num;
						DataGridView3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value), "dd-MM-yyyy");
						DataGridView3.Rows[(int)num].Cells[2].Value = value2;
						DataGridView3.Rows[(int)num].Cells[3].Value = text;
						DataGridView3.Rows[(int)num].Cells[4].Value = text2;
						DataGridView3.Rows[(int)num].Cells[5].Value = Strings.Format(Conversion.Val(text3), "0.00");
						DataGridView3.Rows[(int)num].Cells[6].Value = Strings.Format(Conversion.Val(inputStr), "0.00");
						DataGridView3.Rows[(int)num].Cells[7].Value = Strings.Format(Conversion.Val(inputStr2), "0.00");
						DataGridView3.Rows[(int)num].Cells[8].Value = Strings.Format(Conversion.Val(inputStr3), "0.00");
						DataGridView3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr4), "0.00");
						DataGridView3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr5), "0.00");
						DataGridView3.Rows[(int)num].Cells[11].Value = Strings.UCase(text4);
						DataGridView3.Rows[(int)num].Cells[12].Value = value3;
						num9 = Conversion.Val(num9) + Conversion.Val(text3);
						num10 = Conversion.Val(num10) + Conversion.Val(inputStr);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr2);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr3);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr4);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr5);
						num++;
					}
					num3++;
				}
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[3].Value = "Total : ";
				DataGridView3.Rows[(int)num].Cells[5].Value = num9;
				DataGridView3.Rows[(int)num].Cells[6].Value = num10;
				DataGridView3.Rows[(int)num].Cells[7].Value = num11;
				DataGridView3.Rows[(int)num].Cells[8].Value = num12;
				DataGridView3.Rows[(int)num].Cells[9].Value = num13;
				DataGridView3.Rows[(int)num].Cells[10].Value = num14;
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[0].Value = "Bills Found In GSTR-A But Not In Bahi-Khata";
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				DataGridView3.Rows[(int)num].Height = 50;
				num9 = 0.0;
				num10 = 0.0;
				num11 = 0.0;
				num12 = 0.0;
				num13 = 0.0;
				num14 = 0.0;
				num++;
				PB3.Value = 45;
				int num15 = DataGridView2.RowCount - 1;
				int num16 = 0;
				long num21 = default;
				while (true)
				{
					int num17 = num16;
					num5 = num15;
					if (num17 > num5)
					{
						break;
					}
					try
					{
						if (Operators.ConditionalCompareObjectEqual(DataGridView2.Rows[num16].Cells[1].Value, null, TextCompare: false))
						{
							break;
						}
					}
					catch (Exception ex5)
					{
						ProjectData.SetProjectError(ex5);
						Exception ex6 = ex5;
						if (Operators.CompareString(DataGridView2.Rows[num16].Cells[1].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					string text12 = DataGridView2.Rows[num16].Cells[1].Value.ToString();
					string text13 = DataGridView2.Rows[num16].Cells[3].Value.ToString();
					string value5 = Strings.Format(Conversions.ToDate(DataGridView2.Rows[num16].Cells[4].Value.ToString()), "dd-MM-yyyy");
					string inputStr7 = DataGridView2.Rows[num16].Cells[8].Value.ToString();
					string inputStr8 = DataGridView2.Rows[num16].Cells[10].Value.ToString();
					string inputStr9 = DataGridView2.Rows[num16].Cells[11].Value.ToString();
					string inputStr10 = DataGridView2.Rows[num16].Cells[12].Value.ToString();
					string inputStr11 = DataGridView2.Rows[num16].Cells[13].Value.ToString();
					string value6 = DataGridView2.Rows[num16].Cells[2].Value.ToString();
					string text14 = DataGridView2.Rows[num16].Cells[7].Value.ToString();
					string value7 = DataGridView2.Rows[num16].Cells[5].Value.ToString();
					string input3 = text13;
					input3 = Regex.Replace(input3, "[^\\w\\\\-]", "");
					input3 = input3.Replace("-", "");
					bool flag2 = false;
					int num18 = DataGridView1.RowCount - 1;
					int num19 = 3;
					while (true)
					{
						int num20 = num19;
						num5 = num18;
						if (num20 > num5)
						{
							break;
						}
						try
						{
							if (Operators.CompareString(DataGridView1.Rows[num19].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
							{
								break;
							}
						}
						catch (Exception ex7)
						{
							ProjectData.SetProjectError(ex7);
							Exception ex8 = ex7;
							if (Operators.ConditionalCompareObjectEqual(DataGridView1.Rows[num19].Cells[2].Value, "", TextCompare: false))
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						string left = DataGridView1.Rows[num19].Cells[0].Value.ToString();
						if (Operators.CompareString(left, "", TextCompare: false) == 0)
						{
							break;
						}
						string text15 = DataGridView1.Rows[num19].Cells[1].Value.ToString();
						string value8 = Strings.Format(Conversions.ToDate(DataGridView1.Rows[num19].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string text16 = DataGridView1.Rows[num19].Cells[8].Value.ToString();
						string text17 = DataGridView1.Rows[num19].Cells[9].Value.ToString();
						string text18 = DataGridView1.Rows[num19].Cells[10].Value.ToString();
						string text19 = DataGridView1.Rows[num19].Cells[11].Value.ToString();
						string text20 = DataGridView1.Rows[num19].Cells[12].Value.ToString();
						string left2 = DataGridView1.Rows[num19].Cells[3].Value.ToString();
						string input4 = text15;
						input4 = Regex.Replace(input4, "[^\\w\\\\-]", "");
						input4 = input4.Replace("-", "");
						string inputStr12 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value5), Conversions.ToDate(value8)));
						flag2 = false;
						if ((Operators.CompareString(left, text12, TextCompare: false) == 0) & (Conversion.Val(inputStr12) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input4, input3))
								{
									if ((Operators.CompareString(left, text12, TextCompare: false) == 0) & (Conversion.Val(inputStr12) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag2 = true;
										break;
									}
									flag2 = false;
								}
								else
								{
									flag2 = false;
								}
							}
							else
							{
								if ((Operators.CompareString(left, text12, TextCompare: false) == 0) & (Conversion.Val(inputStr12) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(left2, text14, TextCompare: false) == 0))
								{
									flag2 = true;
									break;
								}
								flag2 = false;
							}
						}
						num19++;
					}
					if (!flag2)
					{
						DataGridView3.RowCount += 1;
						DataGridView3.Rows[(int)num].Cells[0].Value = num21 + 1;
						DataGridView3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value5), "dd-MM-yyyy");
						DataGridView3.Rows[(int)num].Cells[2].Value = value6;
						DataGridView3.Rows[(int)num].Cells[3].Value = text12;
						DataGridView3.Rows[(int)num].Cells[4].Value = text13;
						DataGridView3.Rows[(int)num].Cells[5].Value = Strings.Format(Conversion.Val(text14), "0.00");
						DataGridView3.Rows[(int)num].Cells[6].Value = Strings.Format(Conversion.Val(inputStr7), "0.00");
						DataGridView3.Rows[(int)num].Cells[7].Value = Strings.Format(Conversion.Val(inputStr8), "0.00");
						DataGridView3.Rows[(int)num].Cells[8].Value = Strings.Format(Conversion.Val(inputStr9), "0.00");
						DataGridView3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr10), "0.00");
						DataGridView3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr11), "0.00");
						DataGridView3.Rows[(int)num].Cells[11].Value = value7;
						num9 = Conversion.Val(num9) + Conversion.Val(text14);
						num10 = Conversion.Val(num10) + Conversion.Val(inputStr7);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr8);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr9);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr10);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr11);
						num++;
						num21++;
					}
					num16++;
				}
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[3].Value = "Total : ";
				DataGridView3.Rows[(int)num].Cells[5].Value = num9;
				DataGridView3.Rows[(int)num].Cells[6].Value = num10;
				DataGridView3.Rows[(int)num].Cells[7].Value = num11;
				DataGridView3.Rows[(int)num].Cells[8].Value = num12;
				DataGridView3.Rows[(int)num].Cells[9].Value = num13;
				DataGridView3.Rows[(int)num].Cells[10].Value = num14;
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[0].Value = "Bills Found But Mismatch In Figures";
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				DataGridView3.Rows[(int)num].Height = 50;
				num++;
				PB3.Value = 75;
				int num22 = DataGridView2.RowCount - 1;
				int num23 = 0;
				string value12 = default;
				long num28 = default;
				while (true)
				{
					int num24 = num23;
					num5 = num22;
					if (num24 > num5)
					{
						break;
					}
					try
					{
						if (Operators.ConditionalCompareObjectEqual(DataGridView2.Rows[num23].Cells[1].Value, null, TextCompare: false))
						{
							break;
						}
					}
					catch (Exception ex9)
					{
						ProjectData.SetProjectError(ex9);
						Exception ex10 = ex9;
						if (Operators.CompareString(DataGridView2.Rows[num23].Cells[1].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					string text21 = DataGridView2.Rows[num23].Cells[1].Value.ToString();
					string text22 = DataGridView2.Rows[num23].Cells[3].Value.ToString();
					string value9 = Strings.Format(Conversions.ToDate(DataGridView2.Rows[num23].Cells[4].Value.ToString()), "dd-MM-yyyy");
					string inputStr13 = DataGridView2.Rows[num23].Cells[8].Value.ToString();
					string inputStr14 = DataGridView2.Rows[num23].Cells[10].Value.ToString();
					string inputStr15 = DataGridView2.Rows[num23].Cells[11].Value.ToString();
					string inputStr16 = DataGridView2.Rows[num23].Cells[12].Value.ToString();
					string inputStr17 = DataGridView2.Rows[num23].Cells[13].Value.ToString();
					string value10 = DataGridView2.Rows[num23].Cells[2].Value.ToString();
					string text23 = DataGridView2.Rows[num23].Cells[7].Value.ToString();
					string input5 = text22;
					input5 = Regex.Replace(input5, "[^\\w\\\\-]", "");
					input5 = input5.Replace("-", "");
					bool flag3 = false;
					bool flag4 = false;
					bool flag5 = false;
					bool flag6 = false;
					bool flag7 = false;
					bool flag8 = false;
					bool flag9 = false;
					bool flag10 = false;
					int num25 = DataGridView1.RowCount - 1;
					int num26 = 3;
					while (true)
					{
						int num27 = num26;
						num5 = num25;
						if (num27 > num5)
						{
							break;
						}
						try
						{
							if (Operators.CompareString(DataGridView1.Rows[num26].Cells[3].Value.ToString(), null, TextCompare: false) == 0)
							{
								break;
							}
						}
						catch (Exception ex11)
						{
							ProjectData.SetProjectError(ex11);
							Exception ex12 = ex11;
							if (Operators.CompareString(DataGridView1.Rows[num26].Cells[3].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						string left3 = DataGridView1.Rows[num26].Cells[0].Value.ToString();
						if (Operators.CompareString(left3, "", TextCompare: false) == 0)
						{
							break;
						}
						string text24 = DataGridView1.Rows[num26].Cells[1].Value.ToString();
						string value11 = Strings.Format(Conversions.ToDate(DataGridView1.Rows[num26].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string inputStr18 = DataGridView1.Rows[num26].Cells[8].Value.ToString();
						string inputStr19 = DataGridView1.Rows[num26].Cells[9].Value.ToString();
						string inputStr20 = DataGridView1.Rows[num26].Cells[10].Value.ToString();
						string inputStr21 = DataGridView1.Rows[num26].Cells[11].Value.ToString();
						string inputStr22 = DataGridView1.Rows[num26].Cells[12].Value.ToString();
						string text25 = DataGridView1.Rows[num26].Cells[3].Value.ToString();
						string text26 = DataGridView1.Rows[num26].Cells[4].Value.ToString();
						text26 = Strings.Mid(text26, 4, text26.Length);
						string input6 = text24;
						input6 = Regex.Replace(input6, "[^\\w\\\\-]", "");
						input6 = input6.Replace("-", "");
						string inputStr23 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value9), Conversions.ToDate(value11)));
						flag10 = false;
						if ((Operators.CompareString(left3, text21, TextCompare: false) == 0) & (Conversion.Val(inputStr23) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input6, input5))
								{
									if (Operators.CompareString(Strings.UCase(text26), Strings.UCase(value12), TextCompare: false) != 0)
									{
										flag10 = true;
										flag3 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(text25) - Conversion.Val(text23))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag4 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr18) - Conversion.Val(inputStr13))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag5 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr19) - Conversion.Val(inputStr14))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag6 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr20) - Conversion.Val(inputStr15))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag8 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr21) - Conversion.Val(inputStr16))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag7 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr22) - Conversion.Val(inputStr17))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag9 = true;
									}
									break;
								}
								flag10 = false;
							}
							else
							{
								if ((Operators.CompareString(left3, text21, TextCompare: false) == 0) & (Conversion.Val(inputStr23) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(text25, text23, TextCompare: false) == 0))
								{
									if (Operators.CompareString(Strings.UCase(text26), Strings.UCase(value12), TextCompare: false) != 0)
									{
										flag10 = true;
										flag3 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(text25) - Conversion.Val(text23))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag4 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr18) - Conversion.Val(inputStr13))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag5 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr19) - Conversion.Val(inputStr14))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag6 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr20) - Conversion.Val(inputStr15))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag8 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr21) - Conversion.Val(inputStr16))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag7 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr22) - Conversion.Val(inputStr17))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag9 = true;
									}
									break;
								}
								flag10 = false;
							}
						}
						num26++;
					}
					if (flag10)
					{
						DataGridView3.RowCount += 1;
						DataGridView3.Rows[(int)num].Cells[0].Value = num28 + 1;
						DataGridView3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value9), "dd-MM-yyyy");
						DataGridView3.Rows[(int)num].Cells[2].Value = value10;
						DataGridView3.Rows[(int)num].Cells[3].Value = text21;
						DataGridView3.Rows[(int)num].Cells[4].Value = text22;
						DataGridView3.Rows[(int)num].Cells[5].Value = Strings.Format(Conversion.Val(text23), "0.00");
						DataGridView3.Rows[(int)num].Cells[6].Value = Strings.Format(Conversion.Val(inputStr13), "0.00");
						DataGridView3.Rows[(int)num].Cells[7].Value = Strings.Format(Conversion.Val(inputStr14), "0.00");
						DataGridView3.Rows[(int)num].Cells[8].Value = Strings.Format(Conversion.Val(inputStr15), "0.00");
						DataGridView3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr16), "0.00");
						DataGridView3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr17), "0.00");
						DataGridView3.Rows[(int)num].Cells[11].Value = value12;
						if (flag3)
						{
							DataGridView3.Rows[(int)num].Cells[11].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[11].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag4)
						{
							DataGridView3.Rows[(int)num].Cells[5].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[5].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag5)
						{
							DataGridView3.Rows[(int)num].Cells[6].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[6].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag6)
						{
							DataGridView3.Rows[(int)num].Cells[7].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[7].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag7)
						{
							DataGridView3.Rows[(int)num].Cells[8].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[8].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag8)
						{
							DataGridView3.Rows[(int)num].Cells[9].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[9].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag9)
						{
							DataGridView3.Rows[(int)num].Cells[10].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[10].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						num++;
						num28++;
					}
					num23++;
				}
				DataGridView3.Columns[0].Width = 200;
				int num29 = 1;
				int num30;
				do
				{
					DataGridView3.Columns[num29].AutoSizeMode = (DataGridViewAutoSizeColumnMode)10;
					num29++;
					num30 = num29;
					num5 = 11;
				}
				while (num30 <= num5);
				int num31 = 5;
				int num32;
				do
				{
					DataGridView3.Columns[num31].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)64;
					num31++;
					num32 = num31;
					num5 = 10;
				}
				while (num32 <= num5);
				int num33 = 76;
				int num34;
				do
				{
					PB3.Value = num33;
					Thread.Sleep(10);
					num33++;
					num34 = num33;
					num5 = 100;
				}
				while (num34 <= num5);
				((Control)PB3).Visible = false;
				((Control)this).Cursor = Cursors.Default;
			}
		}

		private void Compare_Third()
		{
			//IL_00ef: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f9: Expected Obj, but got Unknown
			//IL_0e57: Unknown result type (might be due to invalid IL or missing references)
			//IL_0e61: Expected Obj, but got Unknown
			//IL_0eec: Unknown result type (might be due to invalid IL or missing references)
			//IL_0ef6: Expected Obj, but got Unknown
			//IL_1a70: Unknown result type (might be due to invalid IL or missing references)
			//IL_1a7a: Expected Obj, but got Unknown
			//IL_1b05: Unknown result type (might be due to invalid IL or missing references)
			//IL_1b0f: Expected Obj, but got Unknown
			//IL_2809: Unknown result type (might be due to invalid IL or missing references)
			//IL_2813: Expected Obj, but got Unknown
			//IL_2877: Unknown result type (might be due to invalid IL or missing references)
			//IL_2881: Expected Obj, but got Unknown
			//IL_28e5: Unknown result type (might be due to invalid IL or missing references)
			//IL_28ef: Expected Obj, but got Unknown
			//IL_2953: Unknown result type (might be due to invalid IL or missing references)
			//IL_295d: Expected Obj, but got Unknown
			//IL_29c1: Unknown result type (might be due to invalid IL or missing references)
			//IL_29cb: Expected Obj, but got Unknown
			//IL_2a31: Unknown result type (might be due to invalid IL or missing references)
			//IL_2a3b: Expected Obj, but got Unknown
			//IL_2aa1: Unknown result type (might be due to invalid IL or missing references)
			//IL_2aab: Expected Obj, but got Unknown
			((Control)this).Cursor = CreateCursor(cursor_file);
			((Control)PB3).Visible = true;
			PB3.Maximum = 100;
			long num = 0L;
			DataGridView3.ColumnCount = 13;
			DataGridView3.RowCount = 1;
			checked
			{
				DataGridView3.Rows[(int)num].Cells[0].Value = "Bills Found In Bahi-Khata But Not In GSTR-2A";
				DataGridView3.Columns[0].DefaultCellStyle.WrapMode = (DataGridViewTriState)1;
				DataGridView3.Rows[(int)num].Height = 50;
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				num++;
				DataGridView3.Columns[0].HeaderText = "";
				DataGridView3.Columns[0].HeaderText = "SR No.";
				DataGridView3.Columns[1].HeaderText = "Date";
				DataGridView3.Columns[2].HeaderText = "Party Name";
				DataGridView3.Columns[3].HeaderText = "GSTIN";
				DataGridView3.Columns[4].HeaderText = "Bill No.";
				DataGridView3.Columns[5].HeaderText = "Bill Amount";
				DataGridView3.Columns[6].HeaderText = "Taxable Amt";
				DataGridView3.Columns[7].HeaderText = "IGST";
				DataGridView3.Columns[8].HeaderText = "CGST";
				DataGridView3.Columns[9].HeaderText = "SGST";
				DataGridView3.Columns[10].HeaderText = "CESS";
				DataGridView3.Columns[11].HeaderText = "POS";
				DataGridView3.Columns[12].HeaderText = "Contact No";
				PB3.Value = 10;
				int num2 = DataGridView1.RowCount - 1;
				int num3 = 3;
				double num9 = default;
				double num10 = default;
				double num11 = default;
				double num12 = default;
				double num13 = default;
				double num14 = default;
				int num5;
				while (true)
				{
					int num4 = num3;
					num5 = num2;
					if (num4 > num5)
					{
						break;
					}
					try
					{
						if (Operators.CompareString(DataGridView1.Rows[num3].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
						{
							break;
						}
					}
					catch (Exception ex)
					{
						ProjectData.SetProjectError(ex);
						Exception ex2 = ex;
						if (Operators.CompareString(DataGridView1.Rows[num3].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					string text = DataGridView1.Rows[num3].Cells[0].Value.ToString();
					if (Operators.CompareString(text, "", TextCompare: false) == 0)
					{
						break;
					}
					string text2 = DataGridView1.Rows[num3].Cells[1].Value.ToString();
					string value = Strings.Format(Conversions.ToDate(DataGridView1.Rows[num3].Cells[2].Value.ToString()), "dd-MM-yyyy");
					string inputStr = DataGridView1.Rows[num3].Cells[8].Value.ToString();
					string inputStr2 = DataGridView1.Rows[num3].Cells[9].Value.ToString();
					string inputStr3 = DataGridView1.Rows[num3].Cells[10].Value.ToString();
					string inputStr4 = DataGridView1.Rows[num3].Cells[11].Value.ToString();
					string inputStr5 = DataGridView1.Rows[num3].Cells[12].Value.ToString();
					string text3 = DataGridView1.Rows[num3].Cells[3].Value.ToString();
					string text4 = DataGridView1.Rows[num3].Cells[4].Value.ToString();
					text4 = Strings.Mid(text4, 4, text4.Length);
					string value2 = DataGridView1.Rows[num3].Cells[16].Value.ToString();
					string value3 = DataGridView1.Rows[num3].Cells[17].Value.ToString();
					string input = text2;
					input = Regex.Replace(input, "[^\\w\\\\-]", "");
					input = input.Replace("-", "");
					bool flag = false;
					int num6 = DataGridView2.RowCount - 1;
					int num7 = 2;
					while (true)
					{
						int num8 = num7;
						num5 = num6;
						if (num8 > num5)
						{
							break;
						}
						try
						{
							if (Operators.ConditionalCompareObjectEqual(DataGridView2.Rows[num7].Cells[0].Value, null, TextCompare: false))
							{
								break;
							}
						}
						catch (Exception ex3)
						{
							ProjectData.SetProjectError(ex3);
							Exception ex4 = ex3;
							if (Operators.CompareString(DataGridView2.Rows[num7].Cells[0].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						if (!Information.IsDate(DataGridView2.Rows[num7].Cells[4].Value.ToString()))
						{
							break;
						}
						string right = DataGridView2.Rows[num7].Cells[0].Value.ToString();
						string text5 = DataGridView2.Rows[num7].Cells[2].Value.ToString();
						string value4 = Strings.Format(Conversions.ToDate(DataGridView2.Rows[num7].Cells[4].Value.ToString()), "dd-MM-yyyy");
						string text6 = DataGridView2.Rows[num7].Cells[9].Value.ToString();
						string text7 = DataGridView2.Rows[num7].Cells[10].Value.ToString();
						string text8 = DataGridView2.Rows[num7].Cells[11].Value.ToString();
						string text9 = DataGridView2.Rows[num7].Cells[12].Value.ToString();
						string text10 = DataGridView2.Rows[num7].Cells[13].Value.ToString();
						string text11 = DataGridView2.Rows[num7].Cells[1].Value.ToString();
						string right2 = DataGridView2.Rows[num7].Cells[5].Value.ToString();
						string input2 = text5;
						input2 = Regex.Replace(input2, "[^\\w\\\\-]", "");
						input2 = input2.Replace("-", "");
						string inputStr6 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value4), Conversions.ToDate(value)));
						flag = false;
						if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr6) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input, input2))
								{
									if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr6) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag = true;
										break;
									}
									flag = false;
								}
								else
								{
									flag = false;
								}
							}
							else
							{
								if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr6) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(text3, right2, TextCompare: false) == 0))
								{
									flag = true;
									break;
								}
								flag = false;
							}
						}
						num7++;
					}
					if (!flag)
					{
						DataGridView3.RowCount += 1;
						DataGridView3.Rows[(int)num].Cells[0].Value = num;
						DataGridView3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value), "dd-MM-yyyy");
						DataGridView3.Rows[(int)num].Cells[2].Value = value2;
						DataGridView3.Rows[(int)num].Cells[3].Value = text;
						DataGridView3.Rows[(int)num].Cells[4].Value = text2;
						DataGridView3.Rows[(int)num].Cells[5].Value = Strings.Format(Conversion.Val(text3), "0.00");
						DataGridView3.Rows[(int)num].Cells[6].Value = Strings.Format(Conversion.Val(inputStr), "0.00");
						DataGridView3.Rows[(int)num].Cells[7].Value = Strings.Format(Conversion.Val(inputStr2), "0.00");
						DataGridView3.Rows[(int)num].Cells[8].Value = Strings.Format(Conversion.Val(inputStr3), "0.00");
						DataGridView3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr4), "0.00");
						DataGridView3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr5), "0.00");
						DataGridView3.Rows[(int)num].Cells[11].Value = Strings.UCase(text4);
						DataGridView3.Rows[(int)num].Cells[12].Value = Strings.UCase(value3);
						num9 = Conversion.Val(num9) + Conversion.Val(text3);
						num10 = Conversion.Val(num10) + Conversion.Val(inputStr);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr2);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr3);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr4);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr5);
						num++;
					}
					num3++;
				}
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[3].Value = "Total : ";
				DataGridView3.Rows[(int)num].Cells[5].Value = num9;
				DataGridView3.Rows[(int)num].Cells[6].Value = num10;
				DataGridView3.Rows[(int)num].Cells[7].Value = num11;
				DataGridView3.Rows[(int)num].Cells[8].Value = num12;
				DataGridView3.Rows[(int)num].Cells[9].Value = num13;
				DataGridView3.Rows[(int)num].Cells[10].Value = num14;
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[0].Value = "Bills Found In GSTR-A But Not In Bahi-Khata";
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				DataGridView3.Rows[(int)num].Height = 50;
				num9 = 0.0;
				num10 = 0.0;
				num11 = 0.0;
				num12 = 0.0;
				num13 = 0.0;
				num14 = 0.0;
				num++;
				PB3.Value = 45;
				int num15 = DataGridView2.RowCount - 1;
				int num16 = 2;
				long num21 = default;
				string value8 = default;
				while (true)
				{
					int num17 = num16;
					num5 = num15;
					if (num17 > num5)
					{
						break;
					}
					try
					{
						if (Operators.ConditionalCompareObjectEqual(DataGridView2.Rows[num16].Cells[0].Value, null, TextCompare: false))
						{
							break;
						}
					}
					catch (Exception ex5)
					{
						ProjectData.SetProjectError(ex5);
						Exception ex6 = ex5;
						if (Operators.CompareString(DataGridView2.Rows[num16].Cells[0].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					if (!Information.IsDate(DataGridView2.Rows[num16].Cells[4].Value.ToString()))
					{
						break;
					}
					string text12 = DataGridView2.Rows[num16].Cells[0].Value.ToString();
					string text13 = DataGridView2.Rows[num16].Cells[2].Value.ToString();
					string value5 = Strings.Format(Conversions.ToDate(DataGridView2.Rows[num16].Cells[4].Value.ToString()), "dd-MM-yyyy");
					string inputStr7 = DataGridView2.Rows[num16].Cells[9].Value.ToString();
					string inputStr8 = DataGridView2.Rows[num16].Cells[10].Value.ToString();
					string inputStr9 = DataGridView2.Rows[num16].Cells[11].Value.ToString();
					string inputStr10 = DataGridView2.Rows[num16].Cells[12].Value.ToString();
					string inputStr11 = DataGridView2.Rows[num16].Cells[13].Value.ToString();
					string value6 = DataGridView2.Rows[num16].Cells[1].Value.ToString();
					string text14 = DataGridView2.Rows[num16].Cells[5].Value.ToString();
					string input3 = text13;
					input3 = Regex.Replace(input3, "[^\\w\\\\-]", "");
					input3 = input3.Replace("-", "");
					bool flag2 = false;
					int num18 = DataGridView1.RowCount - 1;
					int num19 = 3;
					while (true)
					{
						int num20 = num19;
						num5 = num18;
						if (num20 > num5)
						{
							break;
						}
						try
						{
							if (Operators.CompareString(DataGridView1.Rows[num19].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
							{
								break;
							}
						}
						catch (Exception ex7)
						{
							ProjectData.SetProjectError(ex7);
							Exception ex8 = ex7;
							if (Operators.CompareString(DataGridView1.Rows[num19].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						string left = DataGridView1.Rows[num19].Cells[0].Value.ToString();
						if (Operators.CompareString(left, "", TextCompare: false) == 0)
						{
							break;
						}
						string text15 = DataGridView1.Rows[num19].Cells[1].Value.ToString();
						string value7 = Strings.Format(Conversions.ToDate(DataGridView1.Rows[num19].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string text16 = DataGridView1.Rows[num19].Cells[8].Value.ToString();
						string text17 = DataGridView1.Rows[num19].Cells[9].Value.ToString();
						string text18 = DataGridView1.Rows[num19].Cells[10].Value.ToString();
						string text19 = DataGridView1.Rows[num19].Cells[11].Value.ToString();
						string text20 = DataGridView1.Rows[num19].Cells[12].Value.ToString();
						string left2 = DataGridView1.Rows[num19].Cells[3].Value.ToString();
						string input4 = text15;
						input4 = Regex.Replace(input4, "[^\\w\\\\-]", "");
						input4 = input4.Replace("-", "");
						string inputStr12 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value5), Conversions.ToDate(value7)));
						flag2 = false;
						flag2 = false;
						if ((Operators.CompareString(left, text12, TextCompare: false) == 0) & (Conversion.Val(inputStr12) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input4, input3))
								{
									if ((Operators.CompareString(left, text12, TextCompare: false) == 0) & (Conversion.Val(inputStr12) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag2 = true;
										break;
									}
									flag2 = false;
								}
								else
								{
									flag2 = false;
								}
							}
							else
							{
								if ((Operators.CompareString(left, text12, TextCompare: false) == 0) & (Conversion.Val(inputStr12) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(left2, text14, TextCompare: false) == 0))
								{
									flag2 = true;
									break;
								}
								flag2 = false;
							}
						}
						num19++;
					}
					if (!flag2)
					{
						DataGridView3.RowCount += 1;
						DataGridView3.Rows[(int)num].Cells[0].Value = num21 + 1;
						DataGridView3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value5), "dd-MM-yyyy");
						DataGridView3.Rows[(int)num].Cells[2].Value = value6;
						DataGridView3.Rows[(int)num].Cells[3].Value = text12;
						DataGridView3.Rows[(int)num].Cells[4].Value = text13;
						DataGridView3.Rows[(int)num].Cells[5].Value = Strings.Format(Conversion.Val(text14), "0.00");
						DataGridView3.Rows[(int)num].Cells[6].Value = Strings.Format(Conversion.Val(inputStr7), "0.00");
						DataGridView3.Rows[(int)num].Cells[7].Value = Strings.Format(Conversion.Val(inputStr8), "0.00");
						DataGridView3.Rows[(int)num].Cells[8].Value = Strings.Format(Conversion.Val(inputStr9), "0.00");
						DataGridView3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr10), "0.00");
						DataGridView3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr11), "0.00");
						DataGridView3.Rows[(int)num].Cells[11].Value = value8;
						num9 = Conversion.Val(num9) + Conversion.Val(text14);
						num10 = Conversion.Val(num10) + Conversion.Val(inputStr7);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr8);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr9);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr10);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr11);
						num++;
						num21++;
					}
					num16++;
				}
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[3].Value = "Total : ";
				DataGridView3.Rows[(int)num].Cells[5].Value = num9;
				DataGridView3.Rows[(int)num].Cells[6].Value = num10;
				DataGridView3.Rows[(int)num].Cells[7].Value = num11;
				DataGridView3.Rows[(int)num].Cells[8].Value = num12;
				DataGridView3.Rows[(int)num].Cells[9].Value = num13;
				DataGridView3.Rows[(int)num].Cells[10].Value = num14;
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				DataGridView3.RowCount += 1;
				DataGridView3.Rows[(int)num].Cells[0].Value = "Bills Found But Mismatch In Figures";
				DataGridView3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				DataGridView3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				DataGridView3.Rows[(int)num].Height = 50;
				num++;
				PB3.Value = 75;
				int num22 = DataGridView2.RowCount - 1;
				int num23 = 2;
				string value12 = default;
				long num28 = default;
				while (true)
				{
					int num24 = num23;
					num5 = num22;
					if (num24 > num5)
					{
						break;
					}
					try
					{
						if (Operators.ConditionalCompareObjectEqual(DataGridView2.Rows[num23].Cells[0].Value, null, TextCompare: false))
						{
							break;
						}
					}
					catch (Exception ex9)
					{
						ProjectData.SetProjectError(ex9);
						Exception ex10 = ex9;
						if (Operators.CompareString(DataGridView2.Rows[num23].Cells[0].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					if (!Information.IsDate(DataGridView2.Rows[num23].Cells[4].Value.ToString()))
					{
						break;
					}
					string text21 = DataGridView2.Rows[num23].Cells[0].Value.ToString();
					string text22 = DataGridView2.Rows[num23].Cells[2].Value.ToString();
					string value9 = Strings.Format(Conversions.ToDate(DataGridView2.Rows[num23].Cells[4].Value.ToString()), "dd-MM-yyyy");
					string inputStr13 = DataGridView2.Rows[num23].Cells[9].Value.ToString();
					string inputStr14 = DataGridView2.Rows[num23].Cells[10].Value.ToString();
					string inputStr15 = DataGridView2.Rows[num23].Cells[11].Value.ToString();
					string inputStr16 = DataGridView2.Rows[num23].Cells[12].Value.ToString();
					string inputStr17 = DataGridView2.Rows[num23].Cells[13].Value.ToString();
					string value10 = DataGridView2.Rows[num23].Cells[1].Value.ToString();
					string text23 = DataGridView2.Rows[num23].Cells[5].Value.ToString();
					string input5 = text22;
					input5 = Regex.Replace(input5, "[^\\w\\\\-]", "");
					input5 = input5.Replace("-", "");
					bool flag3 = false;
					bool flag4 = false;
					bool flag5 = false;
					bool flag6 = false;
					bool flag7 = false;
					bool flag8 = false;
					bool flag9 = false;
					bool flag10 = false;
					int num25 = DataGridView1.RowCount - 1;
					int num26 = 3;
					while (true)
					{
						int num27 = num26;
						num5 = num25;
						if (num27 > num5)
						{
							break;
						}
						try
						{
							if (Operators.CompareString(DataGridView1.Rows[num26].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
							{
								break;
							}
						}
						catch (Exception ex11)
						{
							ProjectData.SetProjectError(ex11);
							Exception ex12 = ex11;
							if (Operators.CompareString(DataGridView1.Rows[num26].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						if (!Information.IsDate(DataGridView1.Rows[num26].Cells[2].Value.ToString()))
						{
							break;
						}
						string left3 = DataGridView1.Rows[num26].Cells[0].Value.ToString();
						if (Operators.CompareString(left3, "", TextCompare: false) == 0)
						{
							break;
						}
						string text24 = DataGridView1.Rows[num26].Cells[1].Value.ToString();
						string value11 = Strings.Format(Conversions.ToDate(DataGridView1.Rows[num26].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string inputStr18 = DataGridView1.Rows[num26].Cells[8].Value.ToString();
						string inputStr19 = DataGridView1.Rows[num26].Cells[9].Value.ToString();
						string inputStr20 = DataGridView1.Rows[num26].Cells[10].Value.ToString();
						string inputStr21 = DataGridView1.Rows[num26].Cells[11].Value.ToString();
						string inputStr22 = DataGridView1.Rows[num26].Cells[12].Value.ToString();
						string text25 = DataGridView1.Rows[num26].Cells[3].Value.ToString();
						string text26 = DataGridView1.Rows[num26].Cells[4].Value.ToString();
						text26 = Strings.Mid(text26, 4, text26.Length);
						string input6 = text24;
						input6 = Regex.Replace(input6, "[^\\w\\\\-]", "");
						input6 = input6.Replace("-", "");
						string inputStr23 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value9), Conversions.ToDate(value11)));
						flag10 = false;
						if ((Operators.CompareString(left3, text21, TextCompare: false) == 0) & (Conversion.Val(inputStr23) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input6, input5))
								{
									if (Operators.CompareString(Strings.UCase(text26), Strings.UCase(value12), TextCompare: false) != 0)
									{
										flag10 = true;
										flag3 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(text25) - Conversion.Val(text23))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag4 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr18) - Conversion.Val(inputStr13))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag5 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr19) - Conversion.Val(inputStr14))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag6 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr20) - Conversion.Val(inputStr15))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag8 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr21) - Conversion.Val(inputStr16))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag7 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr22) - Conversion.Val(inputStr17))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag9 = true;
									}
									break;
								}
								flag10 = false;
							}
							else
							{
								if ((Operators.CompareString(left3, text21, TextCompare: false) == 0) & (Conversion.Val(inputStr23) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(text25, text23, TextCompare: false) == 0))
								{
									if (Operators.CompareString(Strings.UCase(text26), Strings.UCase(value12), TextCompare: false) != 0)
									{
										flag10 = true;
										flag3 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(text25) - Conversion.Val(text23))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag4 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr18) - Conversion.Val(inputStr13))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag5 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr19) - Conversion.Val(inputStr14))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag6 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr20) - Conversion.Val(inputStr15))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag8 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr21) - Conversion.Val(inputStr16))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag7 = true;
									}
									if (Math.Abs(Conversion.Val(Conversion.Val(inputStr22) - Conversion.Val(inputStr17))) > Conversion.Val(TxtInfo.Text))
									{
										flag10 = true;
										flag9 = true;
									}
									break;
								}
								flag10 = false;
							}
						}
						num26++;
					}
					if (flag10)
					{
						DataGridView3.RowCount += 1;
						DataGridView3.Rows[(int)num].Cells[0].Value = num28 + 1;
						DataGridView3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value9), "dd-MM-yyyy");
						DataGridView3.Rows[(int)num].Cells[2].Value = value10;
						DataGridView3.Rows[(int)num].Cells[3].Value = text21;
						DataGridView3.Rows[(int)num].Cells[4].Value = text22;
						DataGridView3.Rows[(int)num].Cells[5].Value = Strings.Format(Conversion.Val(text23), "0.00");
						DataGridView3.Rows[(int)num].Cells[6].Value = Strings.Format(Conversion.Val(inputStr13), "0.00");
						DataGridView3.Rows[(int)num].Cells[7].Value = Strings.Format(Conversion.Val(inputStr14), "0.00");
						DataGridView3.Rows[(int)num].Cells[8].Value = Strings.Format(Conversion.Val(inputStr15), "0.00");
						DataGridView3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr16), "0.00");
						DataGridView3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr17), "0.00");
						DataGridView3.Rows[(int)num].Cells[11].Value = value12;
						if (flag3)
						{
							DataGridView3.Rows[(int)num].Cells[11].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[11].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag4)
						{
							DataGridView3.Rows[(int)num].Cells[5].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[5].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag5)
						{
							DataGridView3.Rows[(int)num].Cells[6].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[6].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag6)
						{
							DataGridView3.Rows[(int)num].Cells[7].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[7].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag7)
						{
							DataGridView3.Rows[(int)num].Cells[8].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[8].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag8)
						{
							DataGridView3.Rows[(int)num].Cells[9].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[9].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag9)
						{
							DataGridView3.Rows[(int)num].Cells[10].Style.ForeColor = Color.Red;
							DataGridView3.Rows[(int)num].Cells[10].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						num++;
						num28++;
					}
					num23++;
				}
				DataGridView3.Columns[0].Width = 200;
				int num29 = 1;
				int num30;
				do
				{
					DataGridView3.Columns[num29].AutoSizeMode = (DataGridViewAutoSizeColumnMode)10;
					num29++;
					num30 = num29;
					num5 = 11;
				}
				while (num30 <= num5);
				int num31 = 5;
				int num32;
				do
				{
					DataGridView3.Columns[num31].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)64;
					num31++;
					num32 = num31;
					num5 = 10;
				}
				while (num32 <= num5);
				int num33 = 76;
				int num34;
				do
				{
					PB3.Value = num33;
					Thread.Sleep(10);
					num33++;
					num34 = num33;
					num5 = 100;
				}
				while (num34 <= num5);
				((Control)PB3).Visible = false;
				((Control)this).Cursor = Cursors.Default;
			}
		}

		private void Load_file_Bahikhata(string fname)
		{
			//IL_0101: Unknown result type (might be due to invalid IL or missing references)
			//IL_0107: Expected Obj, but got Unknown
			//IL_011d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0123: Expected Obj, but got Unknown
			Guid clsid = new Guid("00024500-0000-0000-C000-000000000046");
			Application application = (Application)Activator.CreateInstance(Type.GetTypeFromCLSID(clsid));
			Workbook workbook = application.Workbooks.Open(fname, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value);
			string text = Conversions.ToString(NewLateBinding.LateGet(workbook.Worksheets[1], null, "Name", new object[0], null, null, null));
			text += "$";
			workbook.Close(Missing.Value, Missing.Value, Missing.Value);
			application.Quit();
			releaseObject(application);
			releaseObject(workbook);
			text = "b2b$";
			OleDbConnection val = new OleDbConnection("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=" + fname + ";Extended Properties=Excel 12.0;");
			string text2 = "Select * From [" + text + "]";
			OleDbDataAdapter val2 = new OleDbDataAdapter(text2, val);
			DataSet dataSet = new DataSet();
			((DbDataAdapter)(object)val2).Fill(dataSet, "[" + text + "]");
			DataGridView1.DataSource = dataSet;
			DataGridView1.DataMember = "[" + text + "]";
		}

		private void Load_file_GSTR(string fname)
		{
			//IL_0101: Unknown result type (might be due to invalid IL or missing references)
			//IL_0107: Expected Obj, but got Unknown
			//IL_011d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0123: Expected Obj, but got Unknown
			Guid clsid = new Guid("00024500-0000-0000-C000-000000000046");
			Application application = (Application)Activator.CreateInstance(Type.GetTypeFromCLSID(clsid));
			Workbook workbook = application.Workbooks.Open(fname, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value);
			string text = Conversions.ToString(NewLateBinding.LateGet(workbook.Worksheets[1], null, "Name", new object[0], null, null, null));
			text += "$";
			workbook.Close(Missing.Value, Missing.Value, Missing.Value);
			application.Quit();
			releaseObject(application);
			releaseObject(workbook);
			text = "invoice$";
			OleDbConnection val = new OleDbConnection("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=" + fname + ";Extended Properties=Excel 12.0;");
			string text2 = "Select * From [" + text + "]";
			OleDbDataAdapter val2 = new OleDbDataAdapter(text2, val);
			DataSet dataSet = new DataSet();
			((DbDataAdapter)(object)val2).Fill(dataSet, "[" + text + "]");
			DataGridView2.DataSource = dataSet;
			DataGridView2.DataMember = "[" + text + "]";
		}

		private void Button4_Click(object sender, EventArgs e)
		{
			if (Conversion.Val(DataGridView3.Rows.Count) == 0.0)
			{
				Button3_Click(RuntimeHelpers.GetObjectValue(sender), e);
			}
			if (Conversion.Val(grid3.Rows.Count) == 0.0)
			{
				Button7_Click(RuntimeHelpers.GetObjectValue(sender), e);
			}
			Export_Invoice();
		}

		private void Export_Invoice()
		{
			//IL_0022: Unknown result type (might be due to invalid IL or missing references)
			//IL_0028: Invalid comparison between Unknown and I4
			//IL_00b8: Unknown result type (might be due to invalid IL or missing references)
			lblResultfilepath.Text = "Result File name : " + Module1.save_file_nm;
			if ((int)((CommonDialog)FolderBrowserDialog1).ShowDialog() != 1)
			{
				return;
			}
			Guid clsid = new Guid("00024500-0000-0000-C000-000000000046");
			Application application = (Application)Activator.CreateInstance(Type.GetTypeFromCLSID(clsid));
			((Control)this).Cursor = CreateCursor(cursor_file);
			string text = Strings.Format(DateAndTime.Today.Date, "dd-MM-yyyy");
			string filename = FolderBrowserDialog1.SelectedPath + "\\" + Module1.save_file_nm;
			application.DisplayAlerts = false;
			if (application == null)
			{
				MessageBox.Show("Excel is not properly installed!!");
				return;
			}
			((Control)PB3).Visible = true;
			PB3.Maximum = 100;
			object value = Missing.Value;
			Workbook workbook = application.Workbooks.Add(RuntimeHelpers.GetObjectValue(value));
			Worksheet worksheet = (Worksheet)workbook.Sheets["Sheet1"];
			worksheet.Cells[1, 1] = "SR No.";
			worksheet.Cells[1, 2] = "Bill Date";
			worksheet.Cells[1, 3] = "Party Name";
			worksheet.Cells[1, 4] = "GSTIN";
			worksheet.Cells[1, 5] = "Bill No.";
			worksheet.Cells[1, 6] = "Bill Amount";
			worksheet.Cells[1, 7] = "Taxable Amt";
			worksheet.Cells[1, 8] = "IGST";
			worksheet.Cells[1, 9] = "CGST";
			worksheet.Cells[1, 10] = "SGST";
			worksheet.Cells[1, 11] = "CESS";
			worksheet.Cells[1, 12] = "POS";
			worksheet.Cells[1, 13] = "Contact";
			Style style = worksheet.Application.ActiveWorkbook.Styles.Add("NewStyle", Missing.Value);
			Style style2 = worksheet.Application.ActiveWorkbook.Styles.Add("NewStyle1", Missing.Value);
			Style style3 = worksheet.Application.ActiveWorkbook.Styles.Add("NewStyle2", Missing.Value);
			int num = 2;
			DataGridView dataGridView = DataGridView3;
			checked
			{
				int num2 = dataGridView.Rows.Count - 1;
				int num3 = 0;
				int num5;
				while (true)
				{
					int num4 = num3;
					num5 = num2;
					if (num4 > num5)
					{
						break;
					}
					worksheet.Cells[num, 1] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[0].Value);
					worksheet.Cells[num, 2] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[1].Value);
					worksheet.Cells[num, 3] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[2].Value);
					worksheet.Cells[num, 4] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[3].Value);
					worksheet.Cells[num, 5] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[4].Value);
					worksheet.Cells[num, 6] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[5].Value);
					worksheet.Cells[num, 7] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[6].Value);
					worksheet.Cells[num, 8] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[7].Value);
					worksheet.Cells[num, 9] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[8].Value);
					worksheet.Cells[num, 10] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[9].Value);
					worksheet.Cells[num, 11] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[10].Value);
					worksheet.Cells[num, 12] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[11].Value);
					worksheet.Cells[num, 13] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[12].Value);
					if (Operators.ConditionalCompareObjectNotEqual(dataGridView.Rows[num3].Cells[0].Value, null, TextCompare: false))
					{
						if (Operators.CompareString(dataGridView.Rows[num3].Cells[0].Value.ToString(), "Bills Found In Bahi-Khata But Not In GSTR-2A", TextCompare: false) == 0)
						{
							style.Font.Bold = true;
							style.Font.Color = ColorTranslator.ToOle(Color.Green);
							NewLateBinding.LateSetComplex(worksheet.Cells[num, 1], null, "Style", new object[1] { "NewStyle" }, null, null, OptimisticSet: false, RValueBase: true);
						}
						if (Operators.CompareString(dataGridView.Rows[num3].Cells[0].Value.ToString(), "Bills Found In GSTR-A But Not In Bahi-Khata", TextCompare: false) == 0)
						{
							style.Font.Bold = true;
							style.Font.Color = ColorTranslator.ToOle(Color.Green);
							style.Interior.Color = ColorTranslator.ToOle(Color.Yellow);
							NewLateBinding.LateSetComplex(worksheet.Cells[num, 1], null, "Style", new object[1] { "NewStyle" }, null, null, OptimisticSet: false, RValueBase: true);
							style2.Font.Bold = true;
							style2.Font.Color = ColorTranslator.ToOle(Color.Blue);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 4], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 6], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 7], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 8], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 9], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 10], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 11], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
						}
						if (Operators.CompareString(dataGridView.Rows[num3].Cells[0].Value.ToString(), "Bills Found But Mismatch In Figures", TextCompare: false) == 0)
						{
							style.Font.Bold = true;
							style.Font.Color = ColorTranslator.ToOle(Color.Green);
							style.Interior.Color = ColorTranslator.ToOle(Color.Yellow);
							NewLateBinding.LateSetComplex(worksheet.Cells[num, 1], null, "Style", new object[1] { "NewStyle" }, null, null, OptimisticSet: false, RValueBase: true);
							style2.Font.Bold = true;
							style2.Font.Color = ColorTranslator.ToOle(Color.Blue);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 4], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 6], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 7], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 8], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 9], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 10], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 11], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
						}
					}
					int num6 = dataGridView.ColumnCount - 1;
					int num7 = 0;
					while (true)
					{
						int num8 = num7;
						num5 = num6;
						if (num8 > num5)
						{
							break;
						}
						if (Operators.CompareString(dataGridView.Rows[num3].Cells[num7].Style.ForeColor.ToString(), "Color [Red]", TextCompare: false) == 0)
						{
							style3.Font.Color = ColorTranslator.ToOle(Color.Red);
							NewLateBinding.LateSetComplex(worksheet.Cells[num, num7 + 1], null, "Style", new object[1] { "NewStyle2" }, null, null, OptimisticSet: false, RValueBase: true);
						}
						num7++;
					}
					num++;
					num3++;
				}
				dataGridView = null;
				((_Worksheet)worksheet).get_Range((object)"B:B", (object)Missing.Value).NumberFormat = "dd-MM-yyyy";
				worksheet.Columns.AutoFit();
				worksheet.Name = "Invoices";
				PB3.Value = 35;
				if (Conversion.Val(workbook.Sheets.Count) <= 1.0)
				{
					workbook.Sheets.Add(Missing.Value, Missing.Value, Missing.Value, Missing.Value);
					worksheet = (Worksheet)workbook.Sheets["Sheet2"];
				}
				else
				{
					worksheet = (Worksheet)workbook.Sheets["Sheet2"];
				}
				worksheet.Cells[1, 1] = "SR No.";
				worksheet.Cells[1, 2] = "Note Date";
				worksheet.Cells[1, 3] = "Party Name";
				worksheet.Cells[1, 4] = "GSTIN";
				worksheet.Cells[1, 5] = "Note No.";
				worksheet.Cells[1, 6] = "Note Type";
				worksheet.Cells[1, 7] = "Tax Rate";
				worksheet.Cells[1, 8] = "Note Amount";
				worksheet.Cells[1, 9] = "Taxable Amt";
				worksheet.Cells[1, 10] = "IGST";
				worksheet.Cells[1, 11] = "CGST";
				worksheet.Cells[1, 12] = "SGST";
				worksheet.Cells[1, 13] = "CESS";
				worksheet.Cells[1, 14] = "Ag. Invoice No.";
				worksheet.Cells[1, 15] = "Ag. Invoice Date";
				worksheet.Cells[1, 16] = "Pre GST";
				worksheet.Cells[1, 17] = "Contact";
				num3 = 0;
				num = 2;
				DataGridView val = grid3;
				int num9 = val.Rows.Count - 1;
				num3 = 0;
				while (true)
				{
					int num10 = num3;
					num5 = num9;
					if (num10 > num5)
					{
						break;
					}
					worksheet.Cells[num, 1] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[0].Value);
					worksheet.Cells[num, 2] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[1].Value);
					worksheet.Cells[num, 3] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[2].Value);
					worksheet.Cells[num, 4] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[3].Value);
					worksheet.Cells[num, 5] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[4].Value);
					worksheet.Cells[num, 6] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[5].Value);
					worksheet.Cells[num, 7] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[6].Value);
					worksheet.Cells[num, 8] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[7].Value);
					worksheet.Cells[num, 9] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[8].Value);
					worksheet.Cells[num, 10] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[9].Value);
					worksheet.Cells[num, 11] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[10].Value);
					worksheet.Cells[num, 12] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[11].Value);
					worksheet.Cells[num, 13] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[12].Value);
					worksheet.Cells[num, 14] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[13].Value);
					worksheet.Cells[num, 15] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[14].Value);
					worksheet.Cells[num, 16] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[15].Value);
					worksheet.Cells[num, 17] = RuntimeHelpers.GetObjectValue(val.Rows[num3].Cells[16].Value);
					if (Operators.ConditionalCompareObjectNotEqual(val.Rows[num3].Cells[0].Value, null, TextCompare: false))
					{
						if (Operators.CompareString(val.Rows[num3].Cells[0].Value.ToString(), "Notes Found In Bahi-Khata But Not In GSTR-2A", TextCompare: false) == 0)
						{
							style.Font.Bold = true;
							style.Font.Color = ColorTranslator.ToOle(Color.Green);
							NewLateBinding.LateSetComplex(worksheet.Cells[num, 1], null, "Style", new object[1] { "NewStyle" }, null, null, OptimisticSet: false, RValueBase: true);
						}
						if (Operators.CompareString(val.Rows[num3].Cells[0].Value.ToString(), "Notes Found In GSTR-A But Not In Bahi-Khata", TextCompare: false) == 0)
						{
							style.Font.Bold = true;
							style.Font.Color = ColorTranslator.ToOle(Color.Green);
							style.Interior.Color = ColorTranslator.ToOle(Color.Yellow);
							NewLateBinding.LateSetComplex(worksheet.Cells[num, 1], null, "Style", new object[1] { "NewStyle" }, null, null, OptimisticSet: false, RValueBase: true);
							style2.Font.Bold = true;
							style2.Font.Color = ColorTranslator.ToOle(Color.Blue);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 4], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 6], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 7], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 8], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 9], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 10], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 11], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
						}
						if (Operators.CompareString(val.Rows[num3].Cells[0].Value.ToString(), "Notes Found But Mismatch In Figures", TextCompare: false) == 0)
						{
							style.Font.Bold = true;
							style.Font.Color = ColorTranslator.ToOle(Color.Green);
							style.Interior.Color = ColorTranslator.ToOle(Color.Yellow);
							NewLateBinding.LateSetComplex(worksheet.Cells[num, 1], null, "Style", new object[1] { "NewStyle" }, null, null, OptimisticSet: false, RValueBase: true);
							style2.Font.Bold = true;
							style2.Font.Color = ColorTranslator.ToOle(Color.Blue);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 4], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 6], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 7], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 8], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 9], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 10], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
							NewLateBinding.LateSetComplex(worksheet.Cells[num - 1, 11], null, "Style", new object[1] { "NewStyle1" }, null, null, OptimisticSet: false, RValueBase: true);
						}
					}
					int num11 = val.ColumnCount - 1;
					int num12 = 0;
					while (true)
					{
						int num13 = num12;
						num5 = num11;
						if (num13 > num5)
						{
							break;
						}
						if (Operators.CompareString(val.Rows[num3].Cells[num12].Style.ForeColor.ToString(), "Color [Red]", TextCompare: false) == 0)
						{
							style3.Font.Color = ColorTranslator.ToOle(Color.Red);
							NewLateBinding.LateSetComplex(worksheet.Cells[num, num12 + 1], null, "Style", new object[1] { "NewStyle2" }, null, null, OptimisticSet: false, RValueBase: true);
						}
						num12++;
					}
					num++;
					num3++;
				}
				val = null;
				((_Worksheet)worksheet).get_Range((object)"B:B", (object)Missing.Value).NumberFormat = "dd-MM-yyyy";
				worksheet.Columns.AutoFit();
				worksheet.Name = "Notes";
				int num14 = 60;
				int num15;
				do
				{
					PB3.Value = num14;
					Thread.Sleep(10);
					num14++;
					num15 = num14;
					num5 = 100;
				}
				while (num15 <= num5);
				((Control)PB3).Visible = false;
				workbook.SaveAs(filename, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, XlSaveAsAccessMode.xlNoChange, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value);
				workbook.Close(true, RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value));
				application.DisplayAlerts = true;
				application.Quit();
				releaseObject(worksheet);
				releaseObject(workbook);
				releaseObject(application);
				((Control)this).Cursor = Cursors.Default;
			}
			string value2 = Conversions.ToString((int)Interaction.MsgBox("Do you wan to Open Excel File ?", MsgBoxStyle.YesNo | MsgBoxStyle.Question, "Kamra Softwares"));
			if (Conversions.ToDouble(value2) == 6.0)
			{
				clsid = new Guid("00024500-0000-0000-C000-000000000046");
				Application application2 = (Application)Activator.CreateInstance(Type.GetTypeFromCLSID(clsid));
				application2.Visible = true;
				Workbook workbook2 = application2.Workbooks.Open(filename, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value);
			}
		}

		private void Export_NOTES()
		{
			//IL_0022: Unknown result type (might be due to invalid IL or missing references)
			//IL_0028: Invalid comparison between Unknown and I4
			//IL_009c: Unknown result type (might be due to invalid IL or missing references)
			lblResultfilepath.Text = "Result File name : " + Module1.save_file_nm;
			if ((int)((CommonDialog)FolderBrowserDialog1).ShowDialog() != 1)
			{
				return;
			}
			Guid clsid = new Guid("00024500-0000-0000-C000-000000000046");
			Application application = (Application)Activator.CreateInstance(Type.GetTypeFromCLSID(clsid));
			string text = Strings.Format(DateAndTime.Today.Date, "dd-MM-yyyy");
			string text2 = FolderBrowserDialog1.SelectedPath + "\\" + Module1.save_file_nm;
			if (application == null)
			{
				MessageBox.Show("Excel is not properly installed!!");
				return;
			}
			object value = Missing.Value;
			Workbook workbook = application.Workbooks.Add(RuntimeHelpers.GetObjectValue(value));
			Worksheet worksheet = (Worksheet)workbook.Sheets["Sheet1"];
			worksheet.Cells[1, 1] = "SR No.";
			worksheet.Cells[1, 2] = "Note Date";
			worksheet.Cells[1, 3] = "Party Name";
			worksheet.Cells[1, 4] = "GSTIN";
			worksheet.Cells[1, 5] = "Note No.";
			worksheet.Cells[1, 6] = "Note Type";
			worksheet.Cells[1, 7] = "Tax Rate";
			worksheet.Cells[1, 8] = "Note Amount";
			worksheet.Cells[1, 9] = "Taxable Amt";
			worksheet.Cells[1, 10] = "IGST";
			worksheet.Cells[1, 11] = "CGST";
			worksheet.Cells[1, 12] = "SGST";
			worksheet.Cells[1, 13] = "CESS";
			worksheet.Cells[1, 14] = "Ag. Invoice No.";
			worksheet.Cells[1, 15] = "Ag. Invoice Date";
			worksheet.Cells[1, 16] = "Pre GST";
			int num = 2;
			DataGridView dataGridView = DataGridView3;
			checked
			{
				int num2 = dataGridView.Rows.Count - 1;
				int num3 = 0;
				while (true)
				{
					int num4 = num3;
					int num5 = num2;
					if (num4 > num5)
					{
						break;
					}
					worksheet.Cells[num, 1] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[0].Value);
					worksheet.Cells[num, 2] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[1].Value);
					worksheet.Cells[num, 3] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[2].Value);
					worksheet.Cells[num, 4] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[3].Value);
					worksheet.Cells[num, 5] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[4].Value);
					worksheet.Cells[num, 6] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[5].Value);
					worksheet.Cells[num, 7] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[6].Value);
					worksheet.Cells[num, 8] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[7].Value);
					worksheet.Cells[num, 9] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[8].Value);
					worksheet.Cells[num, 10] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[9].Value);
					worksheet.Cells[num, 11] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[10].Value);
					worksheet.Cells[num, 12] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[11].Value);
					worksheet.Cells[num, 13] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[12].Value);
					worksheet.Cells[num, 14] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[13].Value);
					worksheet.Cells[num, 15] = RuntimeHelpers.GetObjectValue(dataGridView.Rows[num3].Cells[14].Value);
					num++;
					num3++;
				}
				dataGridView = null;
				worksheet.Columns.AutoFit();
				worksheet.Name = "Notes";
				workbook.SaveAs(text2, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value, XlSaveAsAccessMode.xlNoChange, Missing.Value, Missing.Value, Missing.Value, Missing.Value, Missing.Value);
				workbook.Close(true, RuntimeHelpers.GetObjectValue(value), RuntimeHelpers.GetObjectValue(value));
				application.Quit();
				releaseObject(worksheet);
				releaseObject(workbook);
				releaseObject(application);
				Interaction.MsgBox("File Saved at : " + text2);
			}
		}

		private void Button5_Click(object sender, EventArgs e)
		{
			Application.Exit();
		}

		private void Button7_Click(object sender, EventArgs e)
		{
			((Control)DataGridView3).Visible = false;
			((Control)grid3).Visible = true;
			((Control)grid3).BringToFront();
			if (first_type)
			{
				Compare_NOTES_Regular_First();
			}
			else if (second_type)
			{
				Compare_NOTES_Other_Second();
			}
			else if (third_type)
			{
				Compare_NOTES_Third();
			}
		}

		private void Compare_NOTES_Regular_First()
		{
			//IL_00ef: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f9: Expected Obj, but got Unknown
			//IL_106c: Unknown result type (might be due to invalid IL or missing references)
			//IL_1076: Expected Obj, but got Unknown
			//IL_1101: Unknown result type (might be due to invalid IL or missing references)
			//IL_110b: Expected Obj, but got Unknown
			//IL_1e6a: Unknown result type (might be due to invalid IL or missing references)
			//IL_1e74: Expected Obj, but got Unknown
			//IL_1eff: Unknown result type (might be due to invalid IL or missing references)
			//IL_1f09: Expected Obj, but got Unknown
			//IL_2b97: Unknown result type (might be due to invalid IL or missing references)
			//IL_2ba1: Expected Obj, but got Unknown
			//IL_2c05: Unknown result type (might be due to invalid IL or missing references)
			//IL_2c0f: Expected Obj, but got Unknown
			//IL_2c75: Unknown result type (might be due to invalid IL or missing references)
			//IL_2c7f: Expected Obj, but got Unknown
			//IL_2ce5: Unknown result type (might be due to invalid IL or missing references)
			//IL_2cef: Expected Obj, but got Unknown
			//IL_2d55: Unknown result type (might be due to invalid IL or missing references)
			//IL_2d5f: Expected Obj, but got Unknown
			//IL_2dc5: Unknown result type (might be due to invalid IL or missing references)
			//IL_2dcf: Expected Obj, but got Unknown
			//IL_2e35: Unknown result type (might be due to invalid IL or missing references)
			//IL_2e3f: Expected Obj, but got Unknown
			//IL_2ea5: Unknown result type (might be due to invalid IL or missing references)
			//IL_2eaf: Expected Obj, but got Unknown
			((Control)this).Cursor = CreateCursor(cursor_file);
			((Control)PB3).Visible = true;
			PB3.Maximum = 100;
			long num = 0L;
			grid3.ColumnCount = 17;
			grid3.RowCount = 1;
			checked
			{
				grid3.Rows[(int)num].Cells[0].Value = "Notes Found In Bahi-Khata But Not In GSTR-2A";
				grid3.Columns[0].DefaultCellStyle.WrapMode = (DataGridViewTriState)1;
				grid3.Rows[(int)num].Height = 50;
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				num++;
				grid3.Columns[0].HeaderText = "";
				grid3.Columns[0].HeaderText = "SR No.";
				grid3.Columns[1].HeaderText = "Date";
				grid3.Columns[2].HeaderText = "Party Name";
				grid3.Columns[3].HeaderText = "GSTIN";
				grid3.Columns[4].HeaderText = "Note No.";
				grid3.Columns[5].HeaderText = "Note Type";
				grid3.Columns[6].HeaderText = "Tax Rate";
				grid3.Columns[7].HeaderText = "Note Amount";
				grid3.Columns[8].HeaderText = "Taxable Amt";
				grid3.Columns[9].HeaderText = "IGST";
				grid3.Columns[10].HeaderText = "CGST";
				grid3.Columns[11].HeaderText = "SGST";
				grid3.Columns[12].HeaderText = "CESS";
				grid3.Columns[13].HeaderText = "Ag. Invoice No.";
				grid3.Columns[14].HeaderText = "Ag. Invoice Date";
				grid3.Columns[15].HeaderText = "Pre GST";
				grid3.Columns[16].HeaderText = "Contact";
				PB3.Value = 10;
				int num2 = grid1.RowCount - 1;
				int num3 = 3;
				double num9 = default;
				double num10 = default;
				double num11 = default;
				double num12 = default;
				double num13 = default;
				double num14 = default;
				int num5;
				while (true)
				{
					int num4 = num3;
					num5 = num2;
					if (num4 > num5)
					{
						break;
					}
					try
					{
						if (Operators.CompareString(grid1.Rows[num3].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
						{
							break;
						}
					}
					catch (Exception ex)
					{
						ProjectData.SetProjectError(ex);
						Exception ex2 = ex;
						if (Operators.CompareString(grid1.Rows[num3].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					string text = grid1.Rows[num3].Cells[0].Value.ToString();
					if (Operators.CompareString(text, "", TextCompare: false) == 0)
					{
						break;
					}
					string text2 = grid1.Rows[num3].Cells[1].Value.ToString();
					string value = Strings.Format(Conversions.ToDate(grid1.Rows[num3].Cells[2].Value.ToString()), "dd-MM-yyyy");
					string value2 = grid1.Rows[num3].Cells[6].Value.ToString();
					string text3 = grid1.Rows[num3].Cells[11].Value.ToString();
					string inputStr = grid1.Rows[num3].Cells[12].Value.ToString();
					string inputStr2 = grid1.Rows[num3].Cells[13].Value.ToString();
					string inputStr3 = grid1.Rows[num3].Cells[14].Value.ToString();
					string inputStr4 = grid1.Rows[num3].Cells[15].Value.ToString();
					string text4 = grid1.Rows[num3].Cells[9].Value.ToString();
					string value3 = grid1.Rows[num3].Cells[10].Value.ToString();
					string value4 = grid1.Rows[num3].Cells[3].Value.ToString();
					string value5 = grid1.Rows[num3].Cells[4].Value.ToString();
					string value6 = grid1.Rows[num3].Cells[5].Value.ToString();
					string value7 = grid1.Rows[num3].Cells[20].Value.ToString();
					string value8 = grid1.Rows[num3].Cells[21].Value.ToString();
					string input = text2;
					input = Regex.Replace(input, "[^\\w\\\\-]", "");
					input = input.Replace("-", "");
					bool flag = false;
					int num6 = grid2.RowCount - 1;
					int num7 = 2;
					while (true)
					{
						int num8 = num7;
						num5 = num6;
						if (num8 > num5)
						{
							break;
						}
						try
						{
							if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num7].Cells[2].Value, null, TextCompare: false))
							{
								break;
							}
						}
						catch (Exception ex3)
						{
							ProjectData.SetProjectError(ex3);
							Exception ex4 = ex3;
							if (Operators.CompareString(grid2.Rows[num7].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						if (!Information.IsDate(grid2.Rows[num7].Cells[7].Value.ToString()))
						{
							break;
						}
						string right = grid2.Rows[num7].Cells[2].Value.ToString();
						string text5 = grid2.Rows[num7].Cells[5].Value.ToString();
						string value9 = Strings.Format(Conversions.ToDate(grid2.Rows[num7].Cells[7].Value.ToString()), "dd-MM-yyyy");
						string text6 = grid2.Rows[num7].Cells[8].Value.ToString();
						string text7 = grid2.Rows[num7].Cells[12].Value.ToString();
						string text8 = grid2.Rows[num7].Cells[14].Value.ToString();
						string text9 = grid2.Rows[num7].Cells[15].Value.ToString();
						string text10 = grid2.Rows[num7].Cells[16].Value.ToString();
						string text11 = grid2.Rows[num7].Cells[17].Value.ToString();
						string text12 = grid2.Rows[num7].Cells[1].Value.ToString();
						string right2 = grid2.Rows[num7].Cells[11].Value.ToString();
						string text13 = grid2.Rows[num7].Cells[9].Value.ToString();
						string text14 = grid2.Rows[num7].Cells[10].Value.ToString();
						string text15 = "";
						string text16 = "";
						string input2 = text5;
						input2 = Regex.Replace(input2, "[^\\w\\\\-]", "");
						input2 = input2.Replace("-", "");
						string inputStr5 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value9), Conversions.ToDate(value)));
						flag = false;
						if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr5) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input, input2))
								{
									if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr5) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag = true;
										break;
									}
									flag = false;
								}
								else
								{
									flag = false;
								}
							}
							else
							{
								if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr5) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(text4, right2, TextCompare: false) == 0))
								{
									flag = true;
									break;
								}
								flag = false;
							}
						}
						num7++;
					}
					if (!flag)
					{
						grid3.RowCount += 1;
						grid3.Rows[(int)num].Cells[0].Value = num;
						grid3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[2].Value = value7;
						grid3.Rows[(int)num].Cells[3].Value = text;
						grid3.Rows[(int)num].Cells[4].Value = text2;
						grid3.Rows[(int)num].Cells[5].Value = value2;
						grid3.Rows[(int)num].Cells[6].Value = value3;
						grid3.Rows[(int)num].Cells[7].Value = text4;
						grid3.Rows[(int)num].Cells[8].Value = text3;
						grid3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr), "0.00");
						grid3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr2), "0.00");
						grid3.Rows[(int)num].Cells[11].Value = Strings.Format(Conversion.Val(inputStr3), "0.00");
						grid3.Rows[(int)num].Cells[12].Value = Strings.Format(Conversion.Val(inputStr4), "0.00");
						grid3.Rows[(int)num].Cells[13].Value = Strings.UCase(value4);
						grid3.Rows[(int)num].Cells[14].Value = Strings.Format(Conversions.ToDate(value5), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[15].Value = value6;
						grid3.Rows[(int)num].Cells[16].Value = value8;
						num9 = Conversion.Val(num9) + Conversion.Val(text4);
						num10 = Conversion.Val(num10) + Conversion.Val(text3);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr2);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr3);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr4);
						num++;
					}
					num3++;
				}
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[3].Value = "Total : ";
				grid3.Rows[(int)num].Cells[5].Value = num9;
				grid3.Rows[(int)num].Cells[6].Value = num10;
				grid3.Rows[(int)num].Cells[7].Value = num11;
				grid3.Rows[(int)num].Cells[8].Value = num12;
				grid3.Rows[(int)num].Cells[9].Value = num13;
				grid3.Rows[(int)num].Cells[10].Value = num14;
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[0].Value = "Notes Found In GSTR-A But Not In Bahi-Khata";
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				grid3.Rows[(int)num].Height = 50;
				num9 = 0.0;
				num10 = 0.0;
				num11 = 0.0;
				num12 = 0.0;
				num13 = 0.0;
				num14 = 0.0;
				num++;
				PB3.Value = 45;
				int num15 = grid2.RowCount - 1;
				int num16 = 2;
				long num21 = default;
				while (true)
				{
					int num17 = num16;
					num5 = num15;
					if (num17 > num5)
					{
						break;
					}
					try
					{
						if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num16].Cells[2].Value, null, TextCompare: false))
						{
							break;
						}
					}
					catch (Exception ex5)
					{
						ProjectData.SetProjectError(ex5);
						Exception ex6 = ex5;
						if (Operators.CompareString(grid2.Rows[num16].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					if (!Information.IsDate(grid2.Rows[num16].Cells[7].Value.ToString()))
					{
						break;
					}
					string text17 = grid2.Rows[num16].Cells[2].Value.ToString();
					string text18 = grid2.Rows[num16].Cells[5].Value.ToString();
					string value10 = Strings.Format(Conversions.ToDate(grid2.Rows[num16].Cells[7].Value.ToString()), "dd-MM-yyyy");
					string value11 = grid2.Rows[num16].Cells[8].Value.ToString();
					string text19 = grid2.Rows[num16].Cells[12].Value.ToString();
					string inputStr6 = grid2.Rows[num16].Cells[14].Value.ToString();
					string inputStr7 = grid2.Rows[num16].Cells[15].Value.ToString();
					string inputStr8 = grid2.Rows[num16].Cells[16].Value.ToString();
					string inputStr9 = grid2.Rows[num16].Cells[17].Value.ToString();
					string value12 = grid2.Rows[num16].Cells[1].Value.ToString();
					string text20 = grid2.Rows[num16].Cells[11].Value.ToString();
					string value13 = grid2.Rows[num16].Cells[9].Value.ToString();
					string value14 = grid2.Rows[num16].Cells[10].Value.ToString();
					string text21 = "";
					string text22 = "";
					string input3 = text18;
					input3 = Regex.Replace(input3, "[^\\w\\\\-]", "");
					input3 = input3.Replace("-", "");
					bool flag2 = false;
					int num18 = grid1.RowCount - 1;
					int num19 = 3;
					while (true)
					{
						int num20 = num19;
						num5 = num18;
						if (num20 > num5)
						{
							break;
						}
						try
						{
							if (Operators.CompareString(grid1.Rows[num19].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
							{
								break;
							}
						}
						catch (Exception ex7)
						{
							ProjectData.SetProjectError(ex7);
							Exception ex8 = ex7;
							if (Operators.CompareString(grid1.Rows[num19].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						string left = grid1.Rows[num19].Cells[0].Value.ToString();
						if (Operators.CompareString(left, "", TextCompare: false) == 0)
						{
							break;
						}
						string text23 = grid1.Rows[num19].Cells[1].Value.ToString();
						string value15 = Strings.Format(Conversions.ToDate(grid1.Rows[num19].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string text24 = grid1.Rows[num19].Cells[6].Value.ToString();
						string text25 = grid1.Rows[num19].Cells[11].Value.ToString();
						string text26 = grid1.Rows[num19].Cells[12].Value.ToString();
						string text27 = grid1.Rows[num19].Cells[13].Value.ToString();
						string text28 = grid1.Rows[num19].Cells[14].Value.ToString();
						string text29 = grid1.Rows[num19].Cells[15].Value.ToString();
						string left2 = grid1.Rows[num19].Cells[9].Value.ToString();
						string text30 = grid1.Rows[num19].Cells[10].Value.ToString();
						string text31 = grid1.Rows[num19].Cells[3].Value.ToString();
						string text32 = grid1.Rows[num19].Cells[4].Value.ToString();
						string text33 = grid1.Rows[num19].Cells[5].Value.ToString();
						string input4 = text23;
						input4 = Regex.Replace(input4, "[^\\w\\\\-]", "");
						input4 = input4.Replace("-", "");
						string inputStr10 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value10), Conversions.ToDate(value15)));
						flag2 = false;
						if ((Operators.CompareString(left, text17, TextCompare: false) == 0) & (Conversion.Val(inputStr10) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input4, input3))
								{
									if ((Operators.CompareString(left, text17, TextCompare: false) == 0) & (Conversion.Val(inputStr10) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag2 = true;
										break;
									}
									flag2 = false;
								}
								else
								{
									flag2 = false;
								}
							}
							else
							{
								if ((Operators.CompareString(left, text17, TextCompare: false) == 0) & (Conversion.Val(inputStr10) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(left2, text20, TextCompare: false) == 0))
								{
									flag2 = true;
									break;
								}
								flag2 = false;
							}
						}
						num19++;
					}
					if (!flag2)
					{
						grid3.RowCount += 1;
						grid3.Rows[(int)num].Cells[0].Value = num;
						grid3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value10), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[2].Value = value12;
						grid3.Rows[(int)num].Cells[3].Value = text17;
						grid3.Rows[(int)num].Cells[4].Value = text18;
						grid3.Rows[(int)num].Cells[5].Value = value11;
						grid3.Rows[(int)num].Cells[6].Value = "";
						grid3.Rows[(int)num].Cells[7].Value = text20;
						grid3.Rows[(int)num].Cells[8].Value = text19;
						grid3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr6), "0.00");
						grid3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr7), "0.00");
						grid3.Rows[(int)num].Cells[11].Value = Strings.Format(Conversion.Val(inputStr8), "0.00");
						grid3.Rows[(int)num].Cells[12].Value = Strings.Format(Conversion.Val(inputStr9), "0.00");
						grid3.Rows[(int)num].Cells[13].Value = Strings.UCase(value13);
						grid3.Rows[(int)num].Cells[14].Value = Strings.Format(Conversions.ToDate(value14), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[15].Value = "";
						num9 = Conversion.Val(num9) + Conversion.Val(text20);
						num10 = Conversion.Val(num10) + Conversion.Val(text19);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr6);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr7);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr8);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr9);
						num++;
						num21++;
					}
					num16++;
				}
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[3].Value = "Total : ";
				grid3.Rows[(int)num].Cells[5].Value = num9;
				grid3.Rows[(int)num].Cells[6].Value = num10;
				grid3.Rows[(int)num].Cells[7].Value = num11;
				grid3.Rows[(int)num].Cells[8].Value = num12;
				grid3.Rows[(int)num].Cells[9].Value = num13;
				grid3.Rows[(int)num].Cells[10].Value = num14;
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[0].Value = "Notes Found But Mismatch In Figures";
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				grid3.Rows[(int)num].Height = 50;
				num++;
				PB3.Value = 75;
				int num22 = grid2.RowCount - 1;
				int num23 = 2;
				long num28 = default;
				while (true)
				{
					int num24 = num23;
					num5 = num22;
					if (num24 > num5)
					{
						break;
					}
					try
					{
						if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num23].Cells[2].Value, null, TextCompare: false))
						{
							break;
						}
					}
					catch (Exception ex9)
					{
						ProjectData.SetProjectError(ex9);
						Exception ex10 = ex9;
						if (Operators.CompareString(grid2.Rows[num23].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					if (!Information.IsDate(grid2.Rows[num23].Cells[7].Value.ToString()))
					{
						break;
					}
					string text34 = grid2.Rows[num23].Cells[2].Value.ToString();
					string text35 = grid2.Rows[num23].Cells[5].Value.ToString();
					string value16 = Strings.Format(Conversions.ToDate(grid2.Rows[num23].Cells[7].Value.ToString()), "dd-MM-yyyy");
					string value17 = grid2.Rows[num23].Cells[8].Value.ToString();
					string text36 = grid2.Rows[num23].Cells[12].Value.ToString();
					string inputStr11 = grid2.Rows[num23].Cells[14].Value.ToString();
					string inputStr12 = grid2.Rows[num23].Cells[15].Value.ToString();
					string inputStr13 = grid2.Rows[num23].Cells[16].Value.ToString();
					string inputStr14 = grid2.Rows[num23].Cells[17].Value.ToString();
					string value18 = grid2.Rows[num23].Cells[1].Value.ToString();
					string text37 = grid2.Rows[num23].Cells[11].Value.ToString();
					string text38 = grid2.Rows[num23].Cells[9].Value.ToString();
					string text39 = grid2.Rows[num23].Cells[10].Value.ToString();
					string text40 = "";
					string text41 = "";
					string input5 = text35;
					input5 = Regex.Replace(input5, "[^\\w\\\\-]", "");
					input5 = input5.Replace("-", "");
					bool flag3 = false;
					bool flag4 = false;
					bool flag5 = false;
					bool flag6 = false;
					bool flag7 = false;
					bool flag8 = false;
					bool flag9 = false;
					bool flag10 = false;
					bool flag11 = false;
					bool flag12 = false;
					int num25 = grid1.RowCount - 1;
					int num26 = 3;
					while (true)
					{
						int num27 = num26;
						num5 = num25;
						if (num27 > num5)
						{
							break;
						}
						try
						{
							if (Operators.CompareString(grid1.Rows[num26].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
							{
								break;
							}
						}
						catch (Exception ex11)
						{
							ProjectData.SetProjectError(ex11);
							Exception ex12 = ex11;
							if (Operators.CompareString(grid1.Rows[num26].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						string left3 = grid1.Rows[num26].Cells[0].Value.ToString();
						if (Operators.CompareString(left3, "", TextCompare: false) == 0)
						{
							break;
						}
						string text42 = grid1.Rows[num26].Cells[1].Value.ToString();
						string value19 = Strings.Format(Conversions.ToDate(grid1.Rows[num26].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string text43 = grid1.Rows[num26].Cells[6].Value.ToString();
						string inputStr15 = grid1.Rows[num26].Cells[11].Value.ToString();
						string inputStr16 = grid1.Rows[num26].Cells[12].Value.ToString();
						string inputStr17 = grid1.Rows[num26].Cells[13].Value.ToString();
						string inputStr18 = grid1.Rows[num26].Cells[14].Value.ToString();
						string inputStr19 = grid1.Rows[num26].Cells[15].Value.ToString();
						string inputStr20 = grid1.Rows[num26].Cells[9].Value.ToString();
						string text44 = grid1.Rows[num26].Cells[10].Value.ToString();
						string inputStr21 = grid1.Rows[num26].Cells[3].Value.ToString();
						string left4 = grid1.Rows[num26].Cells[4].Value.ToString();
						string text45 = grid1.Rows[num26].Cells[5].Value.ToString();
						string input6 = text42;
						input6 = Regex.Replace(input6, "[^\\w\\\\-]", "");
						input6 = input6.Replace("-", "");
						string inputStr22 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value16), Conversions.ToDate(value19)));
						flag12 = false;
						if ((Operators.CompareString(left3, text34, TextCompare: false) == 0) & (Conversion.Val(inputStr22) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr20) - Conversion.Val(text37))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag4 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr15) - Conversion.Val(text36))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag5 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr16) - Conversion.Val(inputStr11))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag6 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr17) - Conversion.Val(inputStr12))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag8 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr18) - Conversion.Val(inputStr13))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag7 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr19) - Conversion.Val(inputStr14))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag9 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr21) != Conversion.Val(text38))) != 0.0)
							{
								flag12 = true;
								flag10 = true;
							}
							if (Operators.CompareString(left4, text39, TextCompare: false) != 0)
							{
								flag12 = true;
								flag11 = true;
							}
							break;
						}
						flag12 = false;
						num26++;
					}
					if (flag12)
					{
						grid3.RowCount += 1;
						grid3.Rows[(int)num].Cells[0].Value = num;
						grid3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value16), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[2].Value = value18;
						grid3.Rows[(int)num].Cells[3].Value = text34;
						grid3.Rows[(int)num].Cells[4].Value = text35;
						grid3.Rows[(int)num].Cells[5].Value = value17;
						grid3.Rows[(int)num].Cells[6].Value = "";
						grid3.Rows[(int)num].Cells[7].Value = text37;
						grid3.Rows[(int)num].Cells[8].Value = text36;
						grid3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr11), "0.00");
						grid3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr12), "0.00");
						grid3.Rows[(int)num].Cells[11].Value = Strings.Format(Conversion.Val(inputStr13), "0.00");
						grid3.Rows[(int)num].Cells[12].Value = Strings.Format(Conversion.Val(inputStr14), "0.00");
						grid3.Rows[(int)num].Cells[13].Value = Strings.UCase(text38);
						grid3.Rows[(int)num].Cells[14].Value = Strings.Format(Conversions.ToDate(text39), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[15].Value = "";
						if (flag4)
						{
							grid3.Rows[(int)num].Cells[7].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[7].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag5)
						{
							grid3.Rows[(int)num].Cells[8].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[8].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag6)
						{
							grid3.Rows[(int)num].Cells[9].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[9].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag7)
						{
							grid3.Rows[(int)num].Cells[10].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[10].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag8)
						{
							grid3.Rows[(int)num].Cells[11].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[11].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag9)
						{
							grid3.Rows[(int)num].Cells[12].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[12].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag10)
						{
							grid3.Rows[(int)num].Cells[13].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[13].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag11)
						{
							grid3.Rows[(int)num].Cells[14].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[14].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						num++;
						num28++;
					}
					num23++;
				}
				grid3.Columns[0].Width = 200;
				int num29 = 1;
				int num30;
				do
				{
					grid3.Columns[num29].AutoSizeMode = (DataGridViewAutoSizeColumnMode)10;
					num29++;
					num30 = num29;
					num5 = 15;
				}
				while (num30 <= num5);
				int num31 = 5;
				int num32;
				do
				{
					grid3.Columns[num31].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)64;
					num31++;
					num32 = num31;
					num5 = 10;
				}
				while (num32 <= num5);
				grid3.Columns[14].DefaultCellStyle.Format = "dd-MM-yyyy";
				int num33 = 76;
				int num34;
				do
				{
					PB3.Value = num33;
					Thread.Sleep(10);
					num33++;
					num34 = num33;
					num5 = 100;
				}
				while (num34 <= num5);
				((Control)PB3).Visible = false;
				((Control)this).Cursor = Cursors.Default;
			}
		}

		private void Compare_NOTES_Other_Second()
		{
			//IL_00ef: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f9: Expected Obj, but got Unknown
			//IL_10aa: Unknown result type (might be due to invalid IL or missing references)
			//IL_10b4: Expected Obj, but got Unknown
			//IL_113f: Unknown result type (might be due to invalid IL or missing references)
			//IL_1149: Expected Obj, but got Unknown
			//IL_1f75: Unknown result type (might be due to invalid IL or missing references)
			//IL_1f7f: Expected Obj, but got Unknown
			//IL_200a: Unknown result type (might be due to invalid IL or missing references)
			//IL_2014: Expected Obj, but got Unknown
			//IL_2ce3: Unknown result type (might be due to invalid IL or missing references)
			//IL_2ced: Expected Obj, but got Unknown
			//IL_2d51: Unknown result type (might be due to invalid IL or missing references)
			//IL_2d5b: Expected Obj, but got Unknown
			//IL_2dc1: Unknown result type (might be due to invalid IL or missing references)
			//IL_2dcb: Expected Obj, but got Unknown
			//IL_2e31: Unknown result type (might be due to invalid IL or missing references)
			//IL_2e3b: Expected Obj, but got Unknown
			//IL_2ea1: Unknown result type (might be due to invalid IL or missing references)
			//IL_2eab: Expected Obj, but got Unknown
			//IL_2f11: Unknown result type (might be due to invalid IL or missing references)
			//IL_2f1b: Expected Obj, but got Unknown
			//IL_2f81: Unknown result type (might be due to invalid IL or missing references)
			//IL_2f8b: Expected Obj, but got Unknown
			//IL_2ff1: Unknown result type (might be due to invalid IL or missing references)
			//IL_2ffb: Expected Obj, but got Unknown
			((Control)this).Cursor = CreateCursor(cursor_file);
			((Control)PB3).Visible = true;
			PB3.Maximum = 100;
			long num = 0L;
			grid3.ColumnCount = 17;
			grid3.RowCount = 1;
			checked
			{
				grid3.Rows[(int)num].Cells[0].Value = "Notes Found In Bahi-Khata But Not In GSTR-2A";
				grid3.Columns[0].DefaultCellStyle.WrapMode = (DataGridViewTriState)1;
				grid3.Rows[(int)num].Height = 50;
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				num++;
				grid3.Columns[0].HeaderText = "";
				grid3.Columns[0].HeaderText = "SR No.";
				grid3.Columns[1].HeaderText = "Date";
				grid3.Columns[2].HeaderText = "Party Name";
				grid3.Columns[3].HeaderText = "GSTIN";
				grid3.Columns[4].HeaderText = "Note No.";
				grid3.Columns[5].HeaderText = "Note Type";
				grid3.Columns[6].HeaderText = "Tax Rate";
				grid3.Columns[7].HeaderText = "Note Amount";
				grid3.Columns[8].HeaderText = "Taxable Amt";
				grid3.Columns[9].HeaderText = "IGST";
				grid3.Columns[10].HeaderText = "CGST";
				grid3.Columns[11].HeaderText = "SGST";
				grid3.Columns[12].HeaderText = "CESS";
				grid3.Columns[13].HeaderText = "Ag. Invoice No.";
				grid3.Columns[14].HeaderText = "Ag. Invoice Date";
				grid3.Columns[15].HeaderText = "Pre GST";
				grid3.Columns[16].HeaderText = "Contact";
				PB3.Value = 10;
				int num2 = grid1.RowCount - 1;
				int num3 = 0;
				double num9 = default;
				double num10 = default;
				double num11 = default;
				double num12 = default;
				double num13 = default;
				double num14 = default;
				int num5;
				while (true)
				{
					int num4 = num3;
					num5 = num2;
					if (num4 > num5)
					{
						break;
					}
					try
					{
						if (Operators.CompareString(grid1.Rows[num3].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
						{
							break;
						}
					}
					catch (Exception ex)
					{
						ProjectData.SetProjectError(ex);
						Exception ex2 = ex;
						if (Operators.CompareString(grid1.Rows[num3].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					string text = grid1.Rows[num3].Cells[0].Value.ToString();
					if (Operators.CompareString(text, "", TextCompare: false) == 0)
					{
						break;
					}
					string text2 = grid1.Rows[num3].Cells[1].Value.ToString();
					string value = Strings.Format(Conversions.ToDate(grid1.Rows[num3].Cells[2].Value.ToString()), "dd-MM-yyyy");
					string value2 = grid1.Rows[num3].Cells[6].Value.ToString();
					string text3 = grid1.Rows[num3].Cells[11].Value.ToString();
					string inputStr = grid1.Rows[num3].Cells[12].Value.ToString();
					string inputStr2 = grid1.Rows[num3].Cells[13].Value.ToString();
					string inputStr3 = grid1.Rows[num3].Cells[14].Value.ToString();
					string inputStr4 = grid1.Rows[num3].Cells[15].Value.ToString();
					string text4 = grid1.Rows[num3].Cells[9].Value.ToString();
					string value3 = grid1.Rows[num3].Cells[10].Value.ToString();
					string value4 = grid1.Rows[num3].Cells[3].Value.ToString();
					string value5 = grid1.Rows[num3].Cells[4].Value.ToString();
					string value6 = grid1.Rows[num3].Cells[5].Value.ToString();
					string value7 = grid1.Rows[num3].Cells[22].Value.ToString();
					string value8 = grid1.Rows[num3].Cells[23].Value.ToString();
					string input = text2;
					input = Regex.Replace(input, "[^\\w\\\\-]", "");
					input = input.Replace("-", "");
					bool flag = false;
					int num6 = grid2.RowCount - 1;
					int num7 = 0;
					while (true)
					{
						int num8 = num7;
						num5 = num6;
						if (num8 > num5)
						{
							break;
						}
						try
						{
							if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num7].Cells[0].Value, null, TextCompare: false))
							{
								break;
							}
						}
						catch (Exception ex3)
						{
							ProjectData.SetProjectError(ex3);
							Exception ex4 = ex3;
							if (Operators.CompareString(grid2.Rows[num7].Cells[0].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num3].Cells[0].Value, null, TextCompare: false))
						{
							break;
						}
						string right = grid2.Rows[num3].Cells[0].Value.ToString();
						string text5 = grid2.Rows[num3].Cells[2].Value.ToString();
						string value9 = Strings.Format(Conversions.ToDate(grid2.Rows[num3].Cells[3].Value.ToString()), "dd-MM-yyyy");
						string text6 = grid2.Rows[num3].Cells[4].Value.ToString();
						string text7 = grid2.Rows[num3].Cells[11].Value.ToString();
						string text8 = grid2.Rows[num3].Cells[13].Value.ToString();
						string text9 = grid2.Rows[num3].Cells[14].Value.ToString();
						string text10 = grid2.Rows[num3].Cells[15].Value.ToString();
						string text11 = grid2.Rows[num3].Cells[16].Value.ToString();
						string text12 = grid2.Rows[num3].Cells[1].Value.ToString();
						string right2 = grid2.Rows[num3].Cells[10].Value.ToString();
						string text13 = grid2.Rows[num3].Cells[5].Value.ToString();
						string text14 = grid2.Rows[num3].Cells[6].Value.ToString();
						string text15 = grid2.Rows[num3].Cells[9].Value.ToString();
						string text16 = grid2.Rows[num3].Cells[12].Value.ToString();
						string input2 = text5;
						input2 = Regex.Replace(input2, "[^\\w\\\\-]", "");
						input2 = input2.Replace("-", "");
						string inputStr5 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value9), Conversions.ToDate(value)));
						flag = false;
						if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr5) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input, input2))
								{
									if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr5) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag = true;
										break;
									}
									flag = false;
								}
								else
								{
									flag = false;
								}
							}
							else
							{
								if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr5) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(text4, right2, TextCompare: false) == 0))
								{
									flag = true;
									break;
								}
								flag = false;
							}
						}
						num7++;
					}
					if (!flag)
					{
						grid3.RowCount += 1;
						grid3.Rows[(int)num].Cells[0].Value = num;
						grid3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[2].Value = value7;
						grid3.Rows[(int)num].Cells[3].Value = text;
						grid3.Rows[(int)num].Cells[4].Value = text2;
						grid3.Rows[(int)num].Cells[5].Value = value2;
						grid3.Rows[(int)num].Cells[6].Value = value3;
						grid3.Rows[(int)num].Cells[7].Value = text4;
						grid3.Rows[(int)num].Cells[8].Value = text3;
						grid3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr), "0.00");
						grid3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr2), "0.00");
						grid3.Rows[(int)num].Cells[11].Value = Strings.Format(Conversion.Val(inputStr3), "0.00");
						grid3.Rows[(int)num].Cells[12].Value = Strings.Format(Conversion.Val(inputStr4), "0.00");
						grid3.Rows[(int)num].Cells[13].Value = Strings.UCase(value4);
						grid3.Rows[(int)num].Cells[14].Value = Strings.Format(Conversions.ToDate(value5), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[15].Value = value6;
						grid3.Rows[(int)num].Cells[16].Value = value8;
						num9 = Conversion.Val(num9) + Conversion.Val(text4);
						num10 = Conversion.Val(num10) + Conversion.Val(text3);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr2);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr3);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr4);
						num++;
					}
					num3++;
				}
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[3].Value = "Total : ";
				grid3.Rows[(int)num].Cells[5].Value = num9;
				grid3.Rows[(int)num].Cells[6].Value = num10;
				grid3.Rows[(int)num].Cells[7].Value = num11;
				grid3.Rows[(int)num].Cells[8].Value = num12;
				grid3.Rows[(int)num].Cells[9].Value = num13;
				grid3.Rows[(int)num].Cells[10].Value = num14;
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[0].Value = "Notes Found In GSTR-A But Not In Bahi-Khata";
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				grid3.Rows[(int)num].Height = 50;
				num9 = 0.0;
				num10 = 0.0;
				num11 = 0.0;
				num12 = 0.0;
				num13 = 0.0;
				num14 = 0.0;
				num++;
				PB3.Value = 45;
				int num15 = grid2.RowCount - 1;
				int num16 = 0;
				long num21 = default;
				while (true)
				{
					int num17 = num16;
					num5 = num15;
					if (num17 > num5)
					{
						break;
					}
					try
					{
						if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num16].Cells[0].Value, null, TextCompare: false))
						{
							break;
						}
					}
					catch (Exception ex5)
					{
						ProjectData.SetProjectError(ex5);
						Exception ex6 = ex5;
						if (Operators.CompareString(grid2.Rows[num16].Cells[0].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num16].Cells[0].Value, null, TextCompare: false))
					{
						break;
					}
					string text17 = grid2.Rows[num16].Cells[0].Value.ToString();
					string text18 = grid2.Rows[num16].Cells[2].Value.ToString();
					string text19 = ((!Information.IsDate(grid2.Rows[num16].Cells[3].Value.ToString())) ? grid2.Rows[num16].Cells[3].Value.ToString() : Strings.Format(Conversions.ToDate(grid2.Rows[num16].Cells[3].Value.ToString()), "dd-MM-yyyy"));
					string value10 = grid2.Rows[num16].Cells[4].Value.ToString();
					string text20 = grid2.Rows[num16].Cells[11].Value.ToString();
					string inputStr6 = grid2.Rows[num16].Cells[13].Value.ToString();
					string inputStr7 = grid2.Rows[num16].Cells[14].Value.ToString();
					string inputStr8 = grid2.Rows[num16].Cells[15].Value.ToString();
					string inputStr9 = grid2.Rows[num16].Cells[16].Value.ToString();
					string value11 = grid2.Rows[num16].Cells[1].Value.ToString();
					string text21 = grid2.Rows[num16].Cells[10].Value.ToString();
					string value12 = grid2.Rows[num16].Cells[5].Value.ToString();
					string value13 = grid2.Rows[num16].Cells[6].Value.ToString();
					string value14 = grid2.Rows[num16].Cells[9].Value.ToString();
					string value15 = grid2.Rows[num16].Cells[12].Value.ToString();
					string input3 = text18;
					input3 = Regex.Replace(input3, "[^\\w\\\\-]", "");
					input3 = input3.Replace("-", "");
					bool flag2 = false;
					int num18 = grid1.RowCount - 1;
					int num19 = 0;
					while (true)
					{
						int num20 = num19;
						num5 = num18;
						if (num20 > num5)
						{
							break;
						}
						try
						{
							if (Operators.CompareString(grid1.Rows[num19].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
							{
								break;
							}
						}
						catch (Exception ex7)
						{
							ProjectData.SetProjectError(ex7);
							Exception ex8 = ex7;
							if (Operators.CompareString(grid1.Rows[num19].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						string left = grid1.Rows[num19].Cells[0].Value.ToString();
						if (Operators.CompareString(left, "", TextCompare: false) == 0)
						{
							break;
						}
						string text22 = grid1.Rows[num19].Cells[1].Value.ToString();
						string value16 = Strings.Format(Conversions.ToDate(grid1.Rows[num19].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string text23 = grid1.Rows[num19].Cells[6].Value.ToString();
						string text24 = grid1.Rows[num19].Cells[11].Value.ToString();
						string text25 = grid1.Rows[num19].Cells[12].Value.ToString();
						string text26 = grid1.Rows[num19].Cells[13].Value.ToString();
						string text27 = grid1.Rows[num19].Cells[14].Value.ToString();
						string text28 = grid1.Rows[num19].Cells[15].Value.ToString();
						string left2 = grid1.Rows[num19].Cells[9].Value.ToString();
						string text29 = grid1.Rows[num19].Cells[10].Value.ToString();
						string text30 = grid1.Rows[num19].Cells[3].Value.ToString();
						string text31 = grid1.Rows[num19].Cells[4].Value.ToString();
						string text32 = grid1.Rows[num19].Cells[5].Value.ToString();
						string input4 = text22;
						input4 = Regex.Replace(input4, "[^\\w\\\\-]", "");
						input4 = input4.Replace("-", "");
						string inputStr10 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(text19), Conversions.ToDate(value16)));
						flag2 = false;
						if ((Operators.CompareString(left, text17, TextCompare: false) == 0) & (Conversion.Val(inputStr10) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input4, input3))
								{
									if ((Operators.CompareString(left, text17, TextCompare: false) == 0) & (Conversion.Val(inputStr10) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag2 = true;
										break;
									}
									flag2 = false;
								}
								else
								{
									flag2 = false;
								}
							}
							else
							{
								if ((Operators.CompareString(left, text17, TextCompare: false) == 0) & (Conversion.Val(inputStr10) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(left2, text21, TextCompare: false) == 0))
								{
									flag2 = true;
									break;
								}
								flag2 = false;
							}
						}
						num19++;
					}
					if (!flag2)
					{
						grid3.RowCount += 1;
						grid3.Rows[(int)num].Cells[0].Value = num;
						if (Information.IsDate(text19))
						{
							grid3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(text19), "dd-MM-yyyy");
						}
						else
						{
							grid3.Rows[(int)num].Cells[1].Value = text19;
						}
						grid3.Rows[(int)num].Cells[2].Value = value11;
						grid3.Rows[(int)num].Cells[3].Value = text17;
						grid3.Rows[(int)num].Cells[4].Value = text18;
						grid3.Rows[(int)num].Cells[5].Value = value10;
						grid3.Rows[(int)num].Cells[6].Value = value15;
						grid3.Rows[(int)num].Cells[7].Value = text21;
						grid3.Rows[(int)num].Cells[8].Value = text20;
						grid3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr6), "0.00");
						grid3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr7), "0.00");
						grid3.Rows[(int)num].Cells[11].Value = Strings.Format(Conversion.Val(inputStr8), "0.00");
						grid3.Rows[(int)num].Cells[12].Value = Strings.Format(Conversion.Val(inputStr9), "0.00");
						grid3.Rows[(int)num].Cells[13].Value = Strings.UCase(value12);
						grid3.Rows[(int)num].Cells[14].Value = Strings.Format(Conversions.ToDate(value13), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[15].Value = value14;
						num9 = Conversion.Val(num9) + Conversion.Val(text21);
						num10 = Conversion.Val(num10) + Conversion.Val(text20);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr6);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr7);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr8);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr9);
						num++;
						num21++;
					}
					num16++;
				}
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[3].Value = "Total : ";
				grid3.Rows[(int)num].Cells[5].Value = num9;
				grid3.Rows[(int)num].Cells[6].Value = num10;
				grid3.Rows[(int)num].Cells[7].Value = num11;
				grid3.Rows[(int)num].Cells[8].Value = num12;
				grid3.Rows[(int)num].Cells[9].Value = num13;
				grid3.Rows[(int)num].Cells[10].Value = num14;
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[0].Value = "Notes Found But Mismatch In Figures";
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				grid3.Rows[(int)num].Height = 50;
				num++;
				PB3.Value = 75;
				int num22 = grid2.RowCount - 1;
				int num23 = 0;
				long num28 = default;
				while (true)
				{
					int num24 = num23;
					num5 = num22;
					if (num24 > num5)
					{
						break;
					}
					try
					{
						if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num23].Cells[0].Value, null, TextCompare: false))
						{
							break;
						}
					}
					catch (Exception ex9)
					{
						ProjectData.SetProjectError(ex9);
						Exception ex10 = ex9;
						if (Operators.CompareString(grid2.Rows[num23].Cells[0].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num23].Cells[0].Value, null, TextCompare: false))
					{
						break;
					}
					string text33 = grid2.Rows[num23].Cells[0].Value.ToString();
					string text34 = grid2.Rows[num23].Cells[2].Value.ToString();
					string value17 = ((!Information.IsDate(grid2.Rows[num23].Cells[3].Value.ToString())) ? grid2.Rows[num23].Cells[3].Value.ToString() : Strings.Format(Conversions.ToDate(grid2.Rows[num23].Cells[3].Value.ToString()), "dd-MM-yyyy"));
					string value18 = grid2.Rows[num23].Cells[4].Value.ToString();
					string text35 = grid2.Rows[num23].Cells[11].Value.ToString();
					string inputStr11 = grid2.Rows[num23].Cells[13].Value.ToString();
					string inputStr12 = grid2.Rows[num23].Cells[14].Value.ToString();
					string inputStr13 = grid2.Rows[num23].Cells[15].Value.ToString();
					string inputStr14 = grid2.Rows[num23].Cells[16].Value.ToString();
					string value19 = grid2.Rows[num23].Cells[1].Value.ToString();
					string text36 = grid2.Rows[num23].Cells[10].Value.ToString();
					string text37 = grid2.Rows[num23].Cells[5].Value.ToString();
					string text38 = grid2.Rows[num23].Cells[6].Value.ToString();
					string value20 = grid2.Rows[num23].Cells[9].Value.ToString();
					string value21 = grid2.Rows[num23].Cells[12].Value.ToString();
					string input5 = text34;
					input5 = Regex.Replace(input5, "[^\\w\\\\-]", "");
					input5 = input5.Replace("-", "");
					bool flag3 = false;
					bool flag4 = false;
					bool flag5 = false;
					bool flag6 = false;
					bool flag7 = false;
					bool flag8 = false;
					bool flag9 = false;
					bool flag10 = false;
					bool flag11 = false;
					bool flag12 = false;
					int num25 = grid1.RowCount - 1;
					int num26 = 0;
					while (true)
					{
						int num27 = num26;
						num5 = num25;
						if (num27 > num5 || Operators.CompareString(grid1.Rows[num26].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							break;
						}
						string left3 = grid1.Rows[num26].Cells[0].Value.ToString();
						if (Operators.CompareString(left3, "", TextCompare: false) == 0)
						{
							break;
						}
						string text39 = grid1.Rows[num26].Cells[1].Value.ToString();
						string value22 = Strings.Format(Conversions.ToDate(grid1.Rows[num26].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string text40 = grid1.Rows[num26].Cells[6].Value.ToString();
						string inputStr15 = grid1.Rows[num26].Cells[11].Value.ToString();
						string inputStr16 = grid1.Rows[num26].Cells[12].Value.ToString();
						string inputStr17 = grid1.Rows[num26].Cells[13].Value.ToString();
						string inputStr18 = grid1.Rows[num26].Cells[14].Value.ToString();
						string inputStr19 = grid1.Rows[num26].Cells[15].Value.ToString();
						string inputStr20 = grid1.Rows[num26].Cells[9].Value.ToString();
						string text41 = grid1.Rows[num26].Cells[10].Value.ToString();
						string inputStr21 = grid1.Rows[num26].Cells[3].Value.ToString();
						string left4 = grid1.Rows[num26].Cells[4].Value.ToString();
						string text42 = grid1.Rows[num26].Cells[5].Value.ToString();
						string input6 = text39;
						input6 = Regex.Replace(input6, "[^\\w\\\\-]", "");
						input6 = input6.Replace("-", "");
						string inputStr22 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value17), Conversions.ToDate(value22)));
						flag12 = false;
						if ((Operators.CompareString(left3, text33, TextCompare: false) == 0) & (Conversion.Val(inputStr22) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr20) - Conversion.Val(text36))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag4 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr15) - Conversion.Val(text35))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag5 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr16) - Conversion.Val(inputStr11))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag6 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr17) - Conversion.Val(inputStr12))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag8 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr18) - Conversion.Val(inputStr13))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag7 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr19) - Conversion.Val(inputStr14))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag9 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr21) != Conversion.Val(text37))) != 0.0)
							{
								flag12 = true;
								flag10 = true;
							}
							if (Operators.CompareString(left4, text38, TextCompare: false) != 0)
							{
								flag12 = true;
								flag10 = true;
							}
							break;
						}
						flag12 = false;
						num26++;
					}
					if (flag12)
					{
						grid3.RowCount += 1;
						grid3.Rows[(int)num].Cells[0].Value = num;
						grid3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value17), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[2].Value = value19;
						grid3.Rows[(int)num].Cells[3].Value = text33;
						grid3.Rows[(int)num].Cells[4].Value = text34;
						grid3.Rows[(int)num].Cells[5].Value = value18;
						grid3.Rows[(int)num].Cells[6].Value = value21;
						grid3.Rows[(int)num].Cells[7].Value = text36;
						grid3.Rows[(int)num].Cells[8].Value = text35;
						grid3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr11), "0.00");
						grid3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr12), "0.00");
						grid3.Rows[(int)num].Cells[11].Value = Strings.Format(Conversion.Val(inputStr13), "0.00");
						grid3.Rows[(int)num].Cells[12].Value = Strings.Format(Conversion.Val(inputStr14), "0.00");
						grid3.Rows[(int)num].Cells[13].Value = Strings.UCase(text37);
						grid3.Rows[(int)num].Cells[14].Value = Strings.Format(Conversions.ToDate(text38), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[15].Value = value20;
						if (flag4)
						{
							grid3.Rows[(int)num].Cells[7].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[7].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag5)
						{
							grid3.Rows[(int)num].Cells[8].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[8].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag6)
						{
							grid3.Rows[(int)num].Cells[9].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[9].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag7)
						{
							grid3.Rows[(int)num].Cells[10].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[10].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag8)
						{
							grid3.Rows[(int)num].Cells[11].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[11].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag9)
						{
							grid3.Rows[(int)num].Cells[12].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[12].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag10)
						{
							grid3.Rows[(int)num].Cells[13].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[13].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag11)
						{
							grid3.Rows[(int)num].Cells[14].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[14].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						num++;
						num28++;
					}
					num23++;
				}
				grid3.Columns[0].Width = 200;
				int num29 = 1;
				int num30;
				do
				{
					grid3.Columns[num29].AutoSizeMode = (DataGridViewAutoSizeColumnMode)10;
					num29++;
					num30 = num29;
					num5 = 15;
				}
				while (num30 <= num5);
				int num31 = 5;
				int num32;
				do
				{
					grid3.Columns[num31].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)64;
					num31++;
					num32 = num31;
					num5 = 10;
				}
				while (num32 <= num5);
				grid3.Columns[14].DefaultCellStyle.Format = "dd-MM-yyyy";
				int num33 = 76;
				int num34;
				do
				{
					PB3.Value = num33;
					Thread.Sleep(10);
					num33++;
					num34 = num33;
					num5 = 100;
				}
				while (num34 <= num5);
				((Control)PB3).Visible = false;
				((Control)this).Cursor = Cursors.Default;
			}
		}

		private void Compare_NOTES_Third()
		{
			//IL_00ef: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f9: Expected Obj, but got Unknown
			//IL_104f: Unknown result type (might be due to invalid IL or missing references)
			//IL_1059: Expected Obj, but got Unknown
			//IL_10e4: Unknown result type (might be due to invalid IL or missing references)
			//IL_10ee: Expected Obj, but got Unknown
			//IL_1e2a: Unknown result type (might be due to invalid IL or missing references)
			//IL_1e34: Expected Obj, but got Unknown
			//IL_1ebf: Unknown result type (might be due to invalid IL or missing references)
			//IL_1ec9: Expected Obj, but got Unknown
			//IL_2ade: Unknown result type (might be due to invalid IL or missing references)
			//IL_2ae8: Expected Obj, but got Unknown
			//IL_2b4c: Unknown result type (might be due to invalid IL or missing references)
			//IL_2b56: Expected Obj, but got Unknown
			//IL_2bbc: Unknown result type (might be due to invalid IL or missing references)
			//IL_2bc6: Expected Obj, but got Unknown
			//IL_2c2c: Unknown result type (might be due to invalid IL or missing references)
			//IL_2c36: Expected Obj, but got Unknown
			//IL_2c9c: Unknown result type (might be due to invalid IL or missing references)
			//IL_2ca6: Expected Obj, but got Unknown
			//IL_2d0c: Unknown result type (might be due to invalid IL or missing references)
			//IL_2d16: Expected Obj, but got Unknown
			//IL_2d7c: Unknown result type (might be due to invalid IL or missing references)
			//IL_2d86: Expected Obj, but got Unknown
			//IL_2dec: Unknown result type (might be due to invalid IL or missing references)
			//IL_2df6: Expected Obj, but got Unknown
			((Control)this).Cursor = CreateCursor(cursor_file);
			((Control)PB3).Visible = true;
			PB3.Maximum = 100;
			long num = 0L;
			grid3.ColumnCount = 17;
			grid3.RowCount = 1;
			checked
			{
				grid3.Rows[(int)num].Cells[0].Value = "Notes Found In Bahi-Khata But Not In GSTR-2A";
				grid3.Columns[0].DefaultCellStyle.WrapMode = (DataGridViewTriState)1;
				grid3.Rows[(int)num].Height = 50;
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				num++;
				grid3.Columns[0].HeaderText = "";
				grid3.Columns[0].HeaderText = "SR No.";
				grid3.Columns[1].HeaderText = "Date";
				grid3.Columns[2].HeaderText = "Party Name";
				grid3.Columns[3].HeaderText = "GSTIN";
				grid3.Columns[4].HeaderText = "Note No.";
				grid3.Columns[5].HeaderText = "Note Type";
				grid3.Columns[6].HeaderText = "Tax Rate";
				grid3.Columns[7].HeaderText = "Note Amount";
				grid3.Columns[8].HeaderText = "Taxable Amt";
				grid3.Columns[9].HeaderText = "IGST";
				grid3.Columns[10].HeaderText = "CGST";
				grid3.Columns[11].HeaderText = "SGST";
				grid3.Columns[12].HeaderText = "CESS";
				grid3.Columns[13].HeaderText = "Ag. Invoice No.";
				grid3.Columns[14].HeaderText = "Ag. Invoice Date";
				grid3.Columns[15].HeaderText = "Pre GST";
				grid3.Columns[16].HeaderText = "Contact";
				PB3.Value = 10;
				int num2 = grid1.RowCount - 1;
				int num3 = 3;
				double num9 = default;
				double num10 = default;
				double num11 = default;
				double num12 = default;
				double num13 = default;
				double num14 = default;
				int num5;
				while (true)
				{
					int num4 = num3;
					num5 = num2;
					if (num4 > num5)
					{
						break;
					}
					try
					{
						if (Operators.CompareString(grid1.Rows[num3].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
						{
							break;
						}
					}
					catch (Exception ex)
					{
						ProjectData.SetProjectError(ex);
						Exception ex2 = ex;
						if (Operators.CompareString(grid1.Rows[num3].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					string text = grid1.Rows[num3].Cells[0].Value.ToString();
					if (Operators.CompareString(text, "", TextCompare: false) == 0)
					{
						break;
					}
					string text2 = grid1.Rows[num3].Cells[1].Value.ToString();
					string value = Strings.Format(Conversions.ToDate(grid1.Rows[num3].Cells[2].Value.ToString()), "dd-MM-yyyy");
					string value2 = grid1.Rows[num3].Cells[6].Value.ToString();
					string text3 = grid1.Rows[num3].Cells[11].Value.ToString();
					string inputStr = grid1.Rows[num3].Cells[12].Value.ToString();
					string inputStr2 = grid1.Rows[num3].Cells[13].Value.ToString();
					string inputStr3 = grid1.Rows[num3].Cells[14].Value.ToString();
					string inputStr4 = grid1.Rows[num3].Cells[15].Value.ToString();
					string text4 = grid1.Rows[num3].Cells[9].Value.ToString();
					string value3 = grid1.Rows[num3].Cells[10].Value.ToString();
					string value4 = grid1.Rows[num3].Cells[3].Value.ToString();
					string value5 = grid1.Rows[num3].Cells[4].Value.ToString();
					string value6 = grid1.Rows[num3].Cells[5].Value.ToString();
					string value7 = grid1.Rows[num3].Cells[20].Value.ToString();
					string value8 = grid1.Rows[num3].Cells[21].Value.ToString();
					string input = text2;
					input = Regex.Replace(input, "[^\\w\\\\-]", "");
					input = input.Replace("-", "");
					bool flag = false;
					int num6 = grid2.RowCount - 1;
					int num7 = 2;
					while (true)
					{
						int num8 = num7;
						num5 = num6;
						if (num8 > num5)
						{
							break;
						}
						try
						{
							if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num7].Cells[0].Value, null, TextCompare: false))
							{
								break;
							}
						}
						catch (Exception ex3)
						{
							ProjectData.SetProjectError(ex3);
							Exception ex4 = ex3;
							if (Operators.CompareString(grid2.Rows[num7].Cells[0].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						string right = grid2.Rows[num7].Cells[0].Value.ToString();
						string text5 = grid2.Rows[num7].Cells[3].Value.ToString();
						string value9 = Strings.Format(Conversions.ToDate(grid2.Rows[num7].Cells[4].Value.ToString()), "dd-MM-yyyy");
						string text6 = grid2.Rows[num7].Cells[2].Value.ToString();
						string text7 = grid2.Rows[num7].Cells[8].Value.ToString();
						string text8 = grid2.Rows[num7].Cells[9].Value.ToString();
						string text9 = grid2.Rows[num7].Cells[10].Value.ToString();
						string text10 = grid2.Rows[num7].Cells[11].Value.ToString();
						string text11 = grid2.Rows[num7].Cells[12].Value.ToString();
						string text12 = grid2.Rows[num7].Cells[1].Value.ToString();
						string right2 = grid2.Rows[num7].Cells[5].Value.ToString();
						string text13 = grid2.Rows[num7].Cells[2].Value.ToString();
						string text14 = grid2.Rows[num7].Cells[3].Value.ToString();
						string text15 = "";
						string text16 = grid2.Rows[num7].Cells[7].Value.ToString();
						string input2 = text5;
						input2 = Regex.Replace(input2, "[^\\w\\\\-]", "");
						input2 = input2.Replace("-", "");
						string inputStr5 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value9), Conversions.ToDate(value)));
						flag = false;
						if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr5) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input, input2))
								{
									if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr5) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag = true;
										break;
									}
									flag = false;
								}
								else
								{
									flag = false;
								}
							}
							else
							{
								if ((Operators.CompareString(text, right, TextCompare: false) == 0) & (Conversion.Val(inputStr5) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(text4, right2, TextCompare: false) == 0))
								{
									flag = true;
									break;
								}
								flag = false;
							}
						}
						num7++;
					}
					if (!flag)
					{
						grid3.RowCount += 1;
						grid3.Rows[(int)num].Cells[0].Value = num;
						grid3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[2].Value = value7;
						grid3.Rows[(int)num].Cells[3].Value = text;
						grid3.Rows[(int)num].Cells[4].Value = text2;
						grid3.Rows[(int)num].Cells[5].Value = value2;
						grid3.Rows[(int)num].Cells[6].Value = value3;
						grid3.Rows[(int)num].Cells[7].Value = text4;
						grid3.Rows[(int)num].Cells[8].Value = text3;
						grid3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr), "0.00");
						grid3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr2), "0.00");
						grid3.Rows[(int)num].Cells[11].Value = Strings.Format(Conversion.Val(inputStr3), "0.00");
						grid3.Rows[(int)num].Cells[12].Value = Strings.Format(Conversion.Val(inputStr4), "0.00");
						grid3.Rows[(int)num].Cells[13].Value = Strings.UCase(value4);
						grid3.Rows[(int)num].Cells[14].Value = Strings.Format(Conversions.ToDate(value5), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[15].Value = value6;
						grid3.Rows[(int)num].Cells[16].Value = value8;
						num9 = Conversion.Val(num9) + Conversion.Val(text4);
						num10 = Conversion.Val(num10) + Conversion.Val(text3);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr2);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr3);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr4);
						num++;
					}
					num3++;
				}
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[3].Value = "Total : ";
				grid3.Rows[(int)num].Cells[5].Value = num9;
				grid3.Rows[(int)num].Cells[6].Value = num10;
				grid3.Rows[(int)num].Cells[7].Value = num11;
				grid3.Rows[(int)num].Cells[8].Value = num12;
				grid3.Rows[(int)num].Cells[9].Value = num13;
				grid3.Rows[(int)num].Cells[10].Value = num14;
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[0].Value = "Notes Found In GSTR-A But Not In Bahi-Khata";
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				grid3.Rows[(int)num].Height = 50;
				num9 = 0.0;
				num10 = 0.0;
				num11 = 0.0;
				num12 = 0.0;
				num13 = 0.0;
				num14 = 0.0;
				num++;
				PB3.Value = 45;
				int num15 = grid2.RowCount - 1;
				int num16 = 2;
				long num21 = default;
				while (true)
				{
					int num17 = num16;
					num5 = num15;
					if (num17 > num5)
					{
						break;
					}
					try
					{
						if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num16].Cells[0].Value, null, TextCompare: false))
						{
							break;
						}
					}
					catch (Exception ex5)
					{
						ProjectData.SetProjectError(ex5);
						Exception ex6 = ex5;
						if (Operators.CompareString(grid2.Rows[num16].Cells[0].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					string text17 = grid2.Rows[num16].Cells[0].Value.ToString();
					string text18 = grid2.Rows[num16].Cells[3].Value.ToString();
					string value10 = Strings.Format(Conversions.ToDate(grid2.Rows[num16].Cells[4].Value.ToString()), "dd-MM-yyyy");
					string value11 = grid2.Rows[num16].Cells[2].Value.ToString();
					string text19 = grid2.Rows[num16].Cells[8].Value.ToString();
					string inputStr6 = grid2.Rows[num16].Cells[9].Value.ToString();
					string inputStr7 = grid2.Rows[num16].Cells[10].Value.ToString();
					string inputStr8 = grid2.Rows[num16].Cells[11].Value.ToString();
					string inputStr9 = grid2.Rows[num16].Cells[12].Value.ToString();
					string value12 = grid2.Rows[num16].Cells[1].Value.ToString();
					string text20 = grid2.Rows[num16].Cells[5].Value.ToString();
					string value13 = grid2.Rows[num16].Cells[2].Value.ToString();
					string value14 = grid2.Rows[num16].Cells[3].Value.ToString();
					string value15 = "";
					string value16 = grid2.Rows[num16].Cells[7].Value.ToString();
					string input3 = text18;
					input3 = Regex.Replace(input3, "[^\\w\\\\-]", "");
					input3 = input3.Replace("-", "");
					bool flag2 = false;
					int num18 = grid1.RowCount - 1;
					int num19 = 3;
					while (true)
					{
						int num20 = num19;
						num5 = num18;
						if (num20 > num5)
						{
							break;
						}
						try
						{
							if (Operators.CompareString(grid1.Rows[num19].Cells[2].Value.ToString(), null, TextCompare: false) == 0)
							{
								break;
							}
						}
						catch (Exception ex7)
						{
							ProjectData.SetProjectError(ex7);
							Exception ex8 = ex7;
							if (Operators.CompareString(grid1.Rows[num19].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
							{
								ProjectData.ClearProjectError();
								break;
							}
							ProjectData.ClearProjectError();
						}
						string left = grid1.Rows[num19].Cells[0].Value.ToString();
						if (Operators.CompareString(left, "", TextCompare: false) == 0)
						{
							break;
						}
						string text21 = grid1.Rows[num19].Cells[1].Value.ToString();
						string value17 = Strings.Format(Conversions.ToDate(grid1.Rows[num19].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string text22 = grid1.Rows[num19].Cells[6].Value.ToString();
						string text23 = grid1.Rows[num19].Cells[11].Value.ToString();
						string text24 = grid1.Rows[num19].Cells[12].Value.ToString();
						string text25 = grid1.Rows[num19].Cells[13].Value.ToString();
						string text26 = grid1.Rows[num19].Cells[14].Value.ToString();
						string text27 = grid1.Rows[num19].Cells[15].Value.ToString();
						string left2 = grid1.Rows[num19].Cells[9].Value.ToString();
						string text28 = grid1.Rows[num19].Cells[10].Value.ToString();
						string text29 = grid1.Rows[num19].Cells[3].Value.ToString();
						string text30 = grid1.Rows[num19].Cells[4].Value.ToString();
						string text31 = grid1.Rows[num19].Cells[5].Value.ToString();
						string input4 = text21;
						input4 = Regex.Replace(input4, "[^\\w\\\\-]", "");
						input4 = input4.Replace("-", "");
						string inputStr10 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value10), Conversions.ToDate(value17)));
						flag2 = false;
						if ((Operators.CompareString(left, text17, TextCompare: false) == 0) & (Conversion.Val(inputStr10) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (RB_Inv_no.Checked)
							{
								if (Invoice_number_match(input4, input3))
								{
									if ((Operators.CompareString(left, text17, TextCompare: false) == 0) & (Conversion.Val(inputStr10) <= Conversion.Val(txtIgnoreDays.Text)))
									{
										flag2 = true;
										break;
									}
									flag2 = false;
								}
								else
								{
									flag2 = false;
								}
							}
							else
							{
								if ((Operators.CompareString(left, text17, TextCompare: false) == 0) & (Conversion.Val(inputStr10) <= Conversion.Val(txtIgnoreDays.Text)) & (Operators.CompareString(left2, text20, TextCompare: false) == 0))
								{
									flag2 = true;
									break;
								}
								flag2 = false;
							}
						}
						num19++;
					}
					if (!flag2)
					{
						grid3.RowCount += 1;
						grid3.Rows[(int)num].Cells[0].Value = num;
						grid3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value10), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[2].Value = value12;
						grid3.Rows[(int)num].Cells[3].Value = text17;
						grid3.Rows[(int)num].Cells[4].Value = text18;
						grid3.Rows[(int)num].Cells[5].Value = value11;
						grid3.Rows[(int)num].Cells[6].Value = value16;
						grid3.Rows[(int)num].Cells[7].Value = text20;
						grid3.Rows[(int)num].Cells[8].Value = text19;
						grid3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr6), "0.00");
						grid3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr7), "0.00");
						grid3.Rows[(int)num].Cells[11].Value = Strings.Format(Conversion.Val(inputStr8), "0.00");
						grid3.Rows[(int)num].Cells[12].Value = Strings.Format(Conversion.Val(inputStr9), "0.00");
						grid3.Rows[(int)num].Cells[13].Value = Strings.UCase(value13);
						grid3.Rows[(int)num].Cells[14].Value = Strings.Format(Conversions.ToDate(value14), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[15].Value = value15;
						num9 = Conversion.Val(num9) + Conversion.Val(text20);
						num10 = Conversion.Val(num10) + Conversion.Val(text19);
						num11 = Conversion.Val(num11) + Conversion.Val(inputStr6);
						num12 = Conversion.Val(num12) + Conversion.Val(inputStr7);
						num13 = Conversion.Val(num13) + Conversion.Val(inputStr8);
						num14 = Conversion.Val(num14) + Conversion.Val(inputStr9);
						num++;
						num21++;
					}
					num16++;
				}
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[3].Value = "Total : ";
				grid3.Rows[(int)num].Cells[5].Value = num9;
				grid3.Rows[(int)num].Cells[6].Value = num10;
				grid3.Rows[(int)num].Cells[7].Value = num11;
				grid3.Rows[(int)num].Cells[8].Value = num12;
				grid3.Rows[(int)num].Cells[9].Value = num13;
				grid3.Rows[(int)num].Cells[10].Value = num14;
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Blue;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 9f, (FontStyle)1);
				num++;
				grid3.RowCount += 1;
				grid3.Rows[(int)num].Cells[0].Value = "Notes Found But Mismatch In Figures";
				grid3.Rows[(int)num].DefaultCellStyle.ForeColor = Color.Green;
				grid3.Rows[(int)num].DefaultCellStyle.Font = new Font("Arial", 8f, (FontStyle)1);
				grid3.Rows[(int)num].Height = 50;
				num++;
				PB3.Value = 75;
				int num22 = grid2.RowCount - 1;
				int num23 = 2;
				long num28 = default;
				while (true)
				{
					int num24 = num23;
					num5 = num22;
					if (num24 > num5)
					{
						break;
					}
					try
					{
						if (Operators.ConditionalCompareObjectEqual(grid2.Rows[num23].Cells[0].Value, null, TextCompare: false))
						{
							break;
						}
					}
					catch (Exception ex9)
					{
						ProjectData.SetProjectError(ex9);
						Exception ex10 = ex9;
						if (Operators.CompareString(grid2.Rows[num23].Cells[0].Value.ToString(), "", TextCompare: false) == 0)
						{
							ProjectData.ClearProjectError();
							break;
						}
						ProjectData.ClearProjectError();
					}
					string text32 = grid2.Rows[num23].Cells[0].Value.ToString();
					string text33 = grid2.Rows[num23].Cells[3].Value.ToString();
					string value18 = Strings.Format(Conversions.ToDate(grid2.Rows[num23].Cells[4].Value.ToString()), "dd-MM-yyyy");
					string value19 = grid2.Rows[num23].Cells[2].Value.ToString();
					string text34 = grid2.Rows[num23].Cells[8].Value.ToString();
					string inputStr11 = grid2.Rows[num23].Cells[9].Value.ToString();
					string inputStr12 = grid2.Rows[num23].Cells[10].Value.ToString();
					string inputStr13 = grid2.Rows[num23].Cells[11].Value.ToString();
					string inputStr14 = grid2.Rows[num23].Cells[12].Value.ToString();
					string value20 = grid2.Rows[num23].Cells[1].Value.ToString();
					string text35 = grid2.Rows[num23].Cells[5].Value.ToString();
					string text36 = grid2.Rows[num23].Cells[2].Value.ToString();
					string text37 = grid2.Rows[num23].Cells[3].Value.ToString();
					string value21 = "";
					string value22 = grid2.Rows[num23].Cells[7].Value.ToString();
					string input5 = text33;
					input5 = Regex.Replace(input5, "[^\\w\\\\-]", "");
					input5 = input5.Replace("-", "");
					bool flag3 = false;
					bool flag4 = false;
					bool flag5 = false;
					bool flag6 = false;
					bool flag7 = false;
					bool flag8 = false;
					bool flag9 = false;
					bool flag10 = false;
					bool flag11 = false;
					bool flag12 = false;
					int num25 = grid1.RowCount - 1;
					int num26 = 3;
					while (true)
					{
						int num27 = num26;
						num5 = num25;
						if (num27 > num5 || Operators.CompareString(grid1.Rows[num26].Cells[2].Value.ToString(), "", TextCompare: false) == 0)
						{
							break;
						}
						string left3 = grid1.Rows[num26].Cells[0].Value.ToString();
						if (Operators.CompareString(left3, "", TextCompare: false) == 0)
						{
							break;
						}
						string text38 = grid1.Rows[num26].Cells[1].Value.ToString();
						string value23 = Strings.Format(Conversions.ToDate(grid1.Rows[num26].Cells[2].Value.ToString()), "dd-MM-yyyy");
						string text39 = grid1.Rows[num26].Cells[6].Value.ToString();
						string inputStr15 = grid1.Rows[num26].Cells[11].Value.ToString();
						string inputStr16 = grid1.Rows[num26].Cells[12].Value.ToString();
						string inputStr17 = grid1.Rows[num26].Cells[13].Value.ToString();
						string inputStr18 = grid1.Rows[num26].Cells[14].Value.ToString();
						string inputStr19 = grid1.Rows[num26].Cells[15].Value.ToString();
						string inputStr20 = grid1.Rows[num26].Cells[9].Value.ToString();
						string text40 = grid1.Rows[num26].Cells[10].Value.ToString();
						string inputStr21 = grid1.Rows[num26].Cells[3].Value.ToString();
						string left4 = grid1.Rows[num26].Cells[4].Value.ToString();
						string text41 = grid1.Rows[num26].Cells[5].Value.ToString();
						string input6 = text38;
						input6 = Regex.Replace(input6, "[^\\w\\\\-]", "");
						input6 = input6.Replace("-", "");
						string inputStr22 = Conversions.ToString(DateAndTime.DateDiff("d", Conversions.ToDate(value18), Conversions.ToDate(value23)));
						flag12 = false;
						if ((Operators.CompareString(left3, text32, TextCompare: false) == 0) & (Conversion.Val(inputStr22) <= Conversion.Val(txtIgnoreDays.Text)))
						{
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr20) - Conversion.Val(text35))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag4 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr15) - Conversion.Val(text34))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag5 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr16) - Conversion.Val(inputStr11))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag6 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr17) - Conversion.Val(inputStr12))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag8 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr18) - Conversion.Val(inputStr13))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag7 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr19) - Conversion.Val(inputStr14))) > Conversion.Val(TxtInfo.Text))
							{
								flag12 = true;
								flag9 = true;
							}
							if (Math.Abs(Conversion.Val(Conversion.Val(inputStr21) != Conversion.Val(text36))) != 0.0)
							{
								flag12 = true;
								flag10 = true;
							}
							if (Operators.CompareString(left4, text37, TextCompare: false) != 0)
							{
								flag12 = true;
								flag11 = true;
							}
							break;
						}
						flag12 = false;
						num26++;
					}
					if (flag12)
					{
						grid3.RowCount += 1;
						grid3.Rows[(int)num].Cells[0].Value = num;
						grid3.Rows[(int)num].Cells[1].Value = Strings.Format(Conversions.ToDate(value18), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[2].Value = value20;
						grid3.Rows[(int)num].Cells[3].Value = text32;
						grid3.Rows[(int)num].Cells[4].Value = text33;
						grid3.Rows[(int)num].Cells[5].Value = value19;
						grid3.Rows[(int)num].Cells[6].Value = value22;
						grid3.Rows[(int)num].Cells[7].Value = text35;
						grid3.Rows[(int)num].Cells[8].Value = text34;
						grid3.Rows[(int)num].Cells[9].Value = Strings.Format(Conversion.Val(inputStr11), "0.00");
						grid3.Rows[(int)num].Cells[10].Value = Strings.Format(Conversion.Val(inputStr12), "0.00");
						grid3.Rows[(int)num].Cells[11].Value = Strings.Format(Conversion.Val(inputStr13), "0.00");
						grid3.Rows[(int)num].Cells[12].Value = Strings.Format(Conversion.Val(inputStr14), "0.00");
						grid3.Rows[(int)num].Cells[13].Value = Strings.UCase(text36);
						grid3.Rows[(int)num].Cells[14].Value = Strings.Format(Conversions.ToDate(text37), "dd-MM-yyyy");
						grid3.Rows[(int)num].Cells[15].Value = value21;
						if (flag4)
						{
							grid3.Rows[(int)num].Cells[7].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[7].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag5)
						{
							grid3.Rows[(int)num].Cells[8].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[8].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag6)
						{
							grid3.Rows[(int)num].Cells[9].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[9].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag7)
						{
							grid3.Rows[(int)num].Cells[10].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[10].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag8)
						{
							grid3.Rows[(int)num].Cells[11].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[11].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag9)
						{
							grid3.Rows[(int)num].Cells[12].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[12].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag10)
						{
							grid3.Rows[(int)num].Cells[13].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[13].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						if (flag11)
						{
							grid3.Rows[(int)num].Cells[14].Style.ForeColor = Color.Red;
							grid3.Rows[(int)num].Cells[14].Style.Font = new Font("Arial", 9f, (FontStyle)1);
						}
						num++;
						num28++;
					}
					num23++;
				}
				grid3.Columns[0].Width = 200;
				int num29 = 1;
				int num30;
				do
				{
					grid3.Columns[num29].AutoSizeMode = (DataGridViewAutoSizeColumnMode)10;
					num29++;
					num30 = num29;
					num5 = 15;
				}
				while (num30 <= num5);
				int num31 = 5;
				int num32;
				do
				{
					grid3.Columns[num31].DefaultCellStyle.Alignment = (DataGridViewContentAlignment)64;
					num31++;
					num32 = num31;
					num5 = 10;
				}
				while (num32 <= num5);
				grid3.Columns[14].DefaultCellStyle.Format = "dd-MM-yyyy";
				int num33 = 76;
				int num34;
				do
				{
					PB3.Value = num33;
					Thread.Sleep(10);
					num33++;
					num34 = num33;
					num5 = 100;
				}
				while (num34 <= num5);
				((Control)PB3).Visible = false;
				((Control)this).Cursor = Cursors.Default;
			}
		}

		private bool Invoice_number_match(string BKno, string TRno)
		{
			string right = Strings.UCase(Strings.Right(TRno, BKno.Length));
			string right2 = Strings.UCase(Strings.Left(BKno, TRno.Length));
			string right3 = Strings.UCase(Strings.Right(BKno, TRno.Length));
			string right4 = Strings.UCase(Strings.Left(TRno, BKno.Length));
			if (Operators.CompareString(Strings.UCase(BKno), Strings.UCase(Strings.Right(TRno, BKno.Length)), TextCompare: false) == 0)
			{
				return true;
			}
			if (Conversion.Val(BKno.Length) < Conversion.Val(TRno.Length))
			{
				if (Operators.CompareString(Strings.UCase(BKno), right, TextCompare: false) == 0)
				{
					return true;
				}
				if (Operators.CompareString(Strings.UCase(TRno), right2, TextCompare: false) == 0)
				{
					return true;
				}
			}
			if (Conversion.Val(BKno.Length) > Conversion.Val(TRno.Length))
			{
				if (Operators.CompareString(Strings.UCase(BKno), right3, TextCompare: false) == 0)
				{
					return true;
				}
				if (Operators.CompareString(Strings.UCase(TRno), right4, TextCompare: false) == 0)
				{
					return true;
				}
			}
			bool result = default;
			return result;
		}

		private void Button6_Click(object sender, EventArgs e)
		{
			string text = "49/2019-20";
			string text2 = "49";
			string right = Strings.UCase(Strings.Right(text2, text.Length));
			string right2 = Strings.UCase(Strings.Left(text, text2.Length));
			string right3 = Strings.UCase(Strings.Right(text, text2.Length));
			string right4 = Strings.UCase(Strings.Left(text2, text.Length));
			if (Operators.CompareString(Strings.UCase(text), right, TextCompare: false) == 0)
			{
				Console.WriteLine("True First");
			}
			else if (Operators.CompareString(Strings.UCase(text2), right2, TextCompare: false) == 0)
			{
				Console.WriteLine("True Second");
			}
			else if (Operators.CompareString(Strings.UCase(text), right3, TextCompare: false) == 0)
			{
				Console.WriteLine("True Third");
			}
			else if (Operators.CompareString(Strings.UCase(text2), right4, TextCompare: false) == 0)
			{
				Console.WriteLine("True Fourth");
			}
		}
	}
	[DesignerGenerated]
	public class Form2 : Form
	{
		private static List<WeakReference> __ENCList = new List<WeakReference>();

		private IContainer components;

		[AccessedThroughProperty("ProgressBar1")]
		private ProgressBar _ProgressBar1;

		[AccessedThroughProperty("Button8")]
		private Button _Button8;

		internal virtual ProgressBar ProgressBar1
		{
			[DebuggerNonUserCode]
			get
			{
				return _ProgressBar1;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				_ProgressBar1 = value;
			}
		}

		internal virtual Button Button8
		{
			[DebuggerNonUserCode]
			get
			{
				return _Button8;
			}
			[MethodImpl(MethodImplOptions.Synchronized)]
			[DebuggerNonUserCode]
			set
			{
				EventHandler eventHandler = Button8_Click_1;
				if (_Button8 != null)
				{
					((Control)_Button8).Click -= eventHandler;
				}
				_Button8 = value;
				if (_Button8 != null)
				{
					((Control)_Button8).Click += eventHandler;
				}
			}
		}

		[DebuggerNonUserCode]
		public Form2()
		{
			((Form)this).Load += Form2_Load;
			__ENCAddToList(this);
			InitializeComponent();
		}

		[DebuggerNonUserCode]
		private static void __ENCAddToList(object value)
		{
			checked
			{
				lock (__ENCList)
				{
					if (__ENCList.Count == __ENCList.Capacity)
					{
						int num = 0;
						int num2 = __ENCList.Count - 1;
						int num3 = 0;
						while (true)
						{
							int num4 = num3;
							int num5 = num2;
							if (num4 > num5)
							{
								break;
							}
							WeakReference weakReference = __ENCList[num3];
							if (weakReference.IsAlive)
							{
								if (num3 != num)
								{
									__ENCList[num] = __ENCList[num3];
								}
								num++;
							}
							num3++;
						}
						__ENCList.RemoveRange(num, __ENCList.Count - num);
						__ENCList.Capacity = __ENCList.Count;
					}
					__ENCList.Add(new WeakReference(RuntimeHelpers.GetObjectValue(value)));
				}
			}
		}

		[DebuggerNonUserCode]
		protected override void Dispose(bool disposing)
		{
			try
			{
				if ((disposing && components != null) ? true : false)
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
			ProgressBar1 = new ProgressBar();
			Button8 = new Button();
			((Control)this).SuspendLayout();
			ProgressBar progressBar = ProgressBar1;
			Point location = new Point(92, 143);
			((Control)progressBar).Location = location;
			((Control)ProgressBar1).Name = "ProgressBar1";
			ProgressBar progressBar2 = ProgressBar1;
			Size size = new Size(100, 23);
			((Control)progressBar2).Size = size;
			((Control)ProgressBar1).TabIndex = 27;
			Button button = Button8;
			location = new Point(104, 94);
			((Control)button).Location = location;
			((Control)Button8).Name = "Button8";
			Button button2 = Button8;
			size = new Size(75, 23);
			((Control)button2).Size = size;
			((Control)Button8).TabIndex = 26;
			((ButtonBase)Button8).Text = "Button8";
			((ButtonBase)Button8).UseVisualStyleBackColor = true;
			SizeF autoScaleDimensions = new SizeF(6f, 13f);
			((ContainerControl)this).AutoScaleDimensions = autoScaleDimensions;
			((ContainerControl)this).AutoScaleMode = (AutoScaleMode)1;
			size = new Size(284, 261);
			((Form)this).ClientSize = size;
			((Control)this).Controls.Add((Control)(object)ProgressBar1);
			((Control)this).Controls.Add((Control)(object)Button8);
			((Control)this).Name = "Form2";
			((Form)this).Text = "Form2";
			((Control)this).ResumeLayout(false);
		}

		private void Form2_Load(object sender, EventArgs e)
		{
			Application.EnableVisualStyles();
		}

		private void Button8_Click_1(object sender, EventArgs e)
		{
			ProgressBar1.Maximum = 100;
			int num = 1;
			checked
			{
				int num2;
				int num3;
				do
				{
					ProgressBar1.Value = (int)Math.Round(Conversion.Val(ProgressBar1.Value) + 1.0);
					num++;
					num2 = num;
					num3 = 100;
				}
				while (num2 <= num3);
			}
		}
	}
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
}
namespace MatchGSTR2.My.Resources
{
	[DebuggerNonUserCode]
	[GeneratedCode("System.Resources.Tools.StronglyTypedResourceBuilder", "4.0.0.0")]
	[StandardModule]
	[HideModuleName]
	[CompilerGenerated]
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
					ResourceManager resourceManager = new ResourceManager("MatchGSTR2.Resources", typeof(Resources).Assembly);
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
namespace MatchGSTR2.My
{
	[EditorBrowsable(EditorBrowsableState.Advanced)]
	[GeneratedCode("Microsoft.VisualStudio.Editors.SettingsDesigner.SettingsSingleFileGenerator", "10.0.0.0")]
	[CompilerGenerated]
	internal sealed class MySettings : ApplicationSettingsBase
	{
		private static MySettings defaultInstance = (MySettings)(object)SettingsBase.Synchronized((SettingsBase)(object)new MySettings());

		public static MySettings Default => defaultInstance;

		[DebuggerNonUserCode]
		public MySettings()
		{
		}
	}
	[CompilerGenerated]
	[DebuggerNonUserCode]
	[HideModuleName]
	[StandardModule]
	internal sealed class MySettingsProperty
	{
		[HelpKeyword("My.Settings")]
		internal static MySettings Settings => MySettings.Default;
	}
}
namespace Microsoft.Office.Interop.Excel
{
	[ComImport]
	[TypeIdentifier]
	[CoClass(typeof(object))]
	[Guid("000208D5-0000-0000-C000-000000000046")]
	[CompilerGenerated]
	public interface Application : _Application, AppEvents_Event
	{
	}
	[ComImport]
	[Guid("000208DA-0000-0000-C000-000000000046")]
	[TypeIdentifier]
	[CoClass(typeof(object))]
	[CompilerGenerated]
	public interface Workbook : _Workbook, WorkbookEvents_Event
	{
	}
	[ComImport]
	[CompilerGenerated]
	[Guid("000208D5-0000-0000-C000-000000000046")]
	[TypeIdentifier]
	[DefaultMember("_Default")]
	public interface _Application
	{
		void _VtblGap1_11();

		[DispId(308)]
		Workbook ActiveWorkbook
		{
			[DispId(308)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		void _VtblGap2_33();

		[DispId(572)]
		Workbooks Workbooks
		{
			[DispId(572)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		void _VtblGap3_60();

		[DispId(0)]
		string _Default
		{
			[DispId(0)]
			[return: MarshalAs(UnmanagedType.BStr)]
			get;
		}

		void _VtblGap4_5();

		[DispId(343)]
		bool DisplayAlerts
		{
			[LCIDConversion(0)]
			[DispId(343)]
			get;
			[LCIDConversion(0)]
			[DispId(343)]
			[param: In]
			set;
		}

		void _VtblGap5_109();

		[DispId(302)]
		void Quit();

		void _VtblGap6_51();

		[DispId(558)]
		bool Visible
		{
			[DispId(558)]
			[LCIDConversion(0)]
			get;
			[LCIDConversion(0)]
			[DispId(558)]
			[param: In]
			set;
		}
	}
	[ComImport]
	[CompilerGenerated]
	[Guid("000208DB-0000-0000-C000-000000000046")]
	[TypeIdentifier]
	public interface Workbooks : IEnumerable
	{
		void _VtblGap1_3();

		[DispId(181)]
		[LCIDConversion(1)]
		[return: MarshalAs(UnmanagedType.Interface)]
		Workbook Add([Optional][In][MarshalAs(UnmanagedType.Struct)] object Template);

		void _VtblGap2_6();

		[IndexerName("_Default")]
		[DispId(0)]
		Workbook this[[In][MarshalAs(UnmanagedType.Struct)] object Index]
		{
			[DispId(0)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		void _VtblGap3_1();

		[LCIDConversion(15)]
		[DispId(1923)]
		[return: MarshalAs(UnmanagedType.Interface)]
		Workbook Open([In][MarshalAs(UnmanagedType.BStr)] string Filename, [Optional][In][MarshalAs(UnmanagedType.Struct)] object UpdateLinks, [Optional][In][MarshalAs(UnmanagedType.Struct)] object ReadOnly, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Format, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Password, [Optional][In][MarshalAs(UnmanagedType.Struct)] object WriteResPassword, [Optional][In][MarshalAs(UnmanagedType.Struct)] object IgnoreReadOnlyRecommended, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Origin, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Delimiter, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Editable, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Notify, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Converter, [Optional][In][MarshalAs(UnmanagedType.Struct)] object AddToMru, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Local, [Optional][In][MarshalAs(UnmanagedType.Struct)] object CorruptLoad);
	}
	[ComImport]
	[Guid("000208DA-0000-0000-C000-000000000046")]
	[TypeIdentifier]
	[CompilerGenerated]
	public interface _Workbook
	{
		void _VtblGap1_20();

		[DispId(277)]
		[LCIDConversion(3)]
		void Close([Optional][In][MarshalAs(UnmanagedType.Struct)] object SaveChanges, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Filename, [Optional][In][MarshalAs(UnmanagedType.Struct)] object RouteWorkbook);

		void _VtblGap2_74();

		[LCIDConversion(0)]
		[DispId(283)]
		void Save();

		void _VtblGap3_9();

		[DispId(485)]
		Sheets Sheets
		{
			[DispId(485)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		void _VtblGap4_2();

		[DispId(493)]
		Styles Styles
		{
			[DispId(493)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		void _VtblGap5_15();

		[DispId(494)]
		Sheets Worksheets
		{
			[DispId(494)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		void _VtblGap6_40();

		[DispId(1925)]
		[LCIDConversion(12)]
		void SaveAs([Optional][In][MarshalAs(UnmanagedType.Struct)] object Filename, [Optional][In][MarshalAs(UnmanagedType.Struct)] object FileFormat, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Password, [Optional][In][MarshalAs(UnmanagedType.Struct)] object WriteResPassword, [Optional][In][MarshalAs(UnmanagedType.Struct)] object ReadOnlyRecommended, [Optional][In][MarshalAs(UnmanagedType.Struct)] object CreateBackup, [Optional][DefaultParameterValue(1)][In] XlSaveAsAccessMode AccessMode, [Optional][In][MarshalAs(UnmanagedType.Struct)] object ConflictResolution, [Optional][In][MarshalAs(UnmanagedType.Struct)] object AddToMru, [Optional][In][MarshalAs(UnmanagedType.Struct)] object TextCodepage, [Optional][In][MarshalAs(UnmanagedType.Struct)] object TextVisualLayout, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Local);
	}
	[ComImport]
	[TypeIdentifier]
	[CompilerGenerated]
	[Guid("000208D7-0000-0000-C000-000000000046")]
	public interface Sheets : IEnumerable
	{
		void _VtblGap1_3();

		[DispId(181)]
		[LCIDConversion(4)]
		[return: MarshalAs(UnmanagedType.IDispatch)]
		object Add([Optional][In][MarshalAs(UnmanagedType.Struct)] object Before, [Optional][In][MarshalAs(UnmanagedType.Struct)] object After, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Count, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Type);

		void _VtblGap2_1();

		[DispId(118)]
		int Count
		{
			[DispId(118)]
			get;
		}

		void _VtblGap3_12();

		[IndexerName("_Default")]
		[DispId(0)]
		object this[[In][MarshalAs(UnmanagedType.Struct)] object Index]
		{
			[DispId(0)]
			[return: MarshalAs(UnmanagedType.IDispatch)]
			get;
		}
	}
	[ComImport]
	[CompilerGenerated]
	[CoClass(typeof(object))]
	[Guid("000208D8-0000-0000-C000-000000000046")]
	[TypeIdentifier]
	public interface Worksheet : _Worksheet, DocEvents_Event
	{
	}
	[ComImport]
	[TypeIdentifier]
	[CompilerGenerated]
	[Guid("000208D8-0000-0000-C000-000000000046")]
	public interface _Worksheet
	{
		[DispId(148)]
		Application Application
		{
			[DispId(148)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		void _VtblGap1_10();

		[DispId(110)]
		string Name
		{
			[DispId(110)]
			[return: MarshalAs(UnmanagedType.BStr)]
			get;
			[DispId(110)]
			[param: In]
			[param: MarshalAs(UnmanagedType.BStr)]
			set;
		}

		void _VtblGap2_32();

		[DispId(238)]
		Range Cells
		{
			[DispId(238)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		void _VtblGap3_5();

		[DispId(241)]
		Range Columns
		{
			[DispId(241)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		void _VtblGap4_41();

		// C# has no syntax for parameterized property 'Range'.
		// Its 'property:' attributes below are ignored by the compiler (CS0657).
		[property: DispId(197)]
		[DispId(197)]
		[return: MarshalAs(UnmanagedType.Interface)]
		Range get_Range([In][MarshalAs(UnmanagedType.Struct)] object Cell1, [Optional][In][MarshalAs(UnmanagedType.Struct)] object Cell2);
	}
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
	[ComImport]
	[InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
	[Guid("00020852-0000-0000-C000-000000000046")]
	[TypeIdentifier]
	[CompilerGenerated]
	[DefaultMember("_Default")]
	public interface Style
	{
		void _VtblGap1_8();

		[DispId(146)]
		Font Font
		{
			[PreserveSig]
			[DispId(146)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		void _VtblGap2_18();

		[DispId(129)]
		Interior Interior
		{
			[PreserveSig]
			[DispId(129)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		void _VtblGap3_19();

		[DispId(0)]
		string _Default
		{
			[PreserveSig]
			[DispId(0)]
			[return: MarshalAs(UnmanagedType.BStr)]
			get;
		}
	}
	[ComImport]
	[InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
	[TypeIdentifier]
	[CompilerGenerated]
	[Guid("00020853-0000-0000-C000-000000000046")]
	public interface Styles
	{
		void _VtblGap1_3();

		[PreserveSig]
		[DispId(181)]
		[return: MarshalAs(UnmanagedType.Interface)]
		Style Add([In][MarshalAs(UnmanagedType.BStr)] string Name, [Optional][In][MarshalAs(UnmanagedType.Struct)] object BasedOn);

		void _VtblGap2_4();

		[IndexerName("_Default")]
		[DispId(0)]
		Style this[[In][MarshalAs(UnmanagedType.Struct)] object Index]
		{
			[PreserveSig]
			[DispId(0)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}
	}
	[ComImport]
	[CompilerGenerated]
	[Guid("0002084D-0000-0000-C000-000000000046")]
	[TypeIdentifier]
	[InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
	public interface Font
	{
		void _VtblGap1_5();

		[DispId(96)]
		object Bold
		{
			[PreserveSig]
			[DispId(96)]
			[return: MarshalAs(UnmanagedType.Struct)]
			get;
			[PreserveSig]
			[DispId(96)]
			[param: In]
			[param: MarshalAs(UnmanagedType.Struct)]
			set;
		}

		[DispId(99)]
		object Color
		{
			[PreserveSig]
			[DispId(99)]
			[return: MarshalAs(UnmanagedType.Struct)]
			get;
			[PreserveSig]
			[DispId(99)]
			[param: In]
			[param: MarshalAs(UnmanagedType.Struct)]
			set;
		}
	}
	[ComImport]
	[TypeIdentifier]
	[CompilerGenerated]
	[InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
	[Guid("00020870-0000-0000-C000-000000000046")]
	public interface Interior
	{
		void _VtblGap1_3();

		[DispId(99)]
		object Color
		{
			[PreserveSig]
			[DispId(99)]
			[return: MarshalAs(UnmanagedType.Struct)]
			get;
			[PreserveSig]
			[DispId(99)]
			[param: In]
			[param: MarshalAs(UnmanagedType.Struct)]
			set;
		}
	}
	[CompilerGenerated]
	[TypeIdentifier("00020813-0000-0000-c000-000000000046", "Microsoft.Office.Interop.Excel.XlSaveAsAccessMode")]
	public enum XlSaveAsAccessMode
	{
		xlExclusive = 3,
		xlNoChange = 1,
		xlShared = 2
	}
	[ComImport]
	[CompilerGenerated]
	[InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
	[Guid("00024411-0000-0000-C000-000000000046")]
	[TypeIdentifier]
	public interface DocEvents
	{
	}
	[ComImport]
	[CompilerGenerated]
	[ComEventInterface(typeof(WorkbookEvents), typeof(WorkbookEvents))]
	[TypeIdentifier("00020813-0000-0000-c000-000000000046", "Microsoft.Office.Interop.Excel.WorkbookEvents_Event")]
	public interface WorkbookEvents_Event
	{
	}
	[ComImport]
	[InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
	[TypeIdentifier]
	[CompilerGenerated]
	[Guid("00024412-0000-0000-C000-000000000046")]
	public interface WorkbookEvents
	{
	}
	[ComImport]
	[CompilerGenerated]
	[Guid("00024413-0000-0000-C000-000000000046")]
	[InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
	[TypeIdentifier]
	public interface AppEvents
	{
	}
	[ComImport]
	[TypeIdentifier("00020813-0000-0000-c000-000000000046", "Microsoft.Office.Interop.Excel.AppEvents_Event")]
	[CompilerGenerated]
	[ComEventInterface(typeof(AppEvents), typeof(AppEvents))]
	public interface AppEvents_Event
	{
	}
	[ComImport]
	[CompilerGenerated]
	[ComEventInterface(typeof(DocEvents), typeof(DocEvents))]
	[TypeIdentifier("00020813-0000-0000-c000-000000000046", "Microsoft.Office.Interop.Excel.DocEvents_Event")]
	public interface DocEvents_Event
	{
	}
}
