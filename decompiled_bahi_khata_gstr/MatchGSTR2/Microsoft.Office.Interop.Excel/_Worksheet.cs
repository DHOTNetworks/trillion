using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Microsoft.Office.Interop.Excel;

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
