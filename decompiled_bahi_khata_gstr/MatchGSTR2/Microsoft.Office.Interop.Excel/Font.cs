using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Microsoft.Office.Interop.Excel;

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
