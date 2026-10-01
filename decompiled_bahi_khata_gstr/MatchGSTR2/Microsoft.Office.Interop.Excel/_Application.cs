using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Microsoft.Office.Interop.Excel;

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
