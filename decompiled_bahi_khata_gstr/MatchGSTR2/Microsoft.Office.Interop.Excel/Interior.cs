using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Microsoft.Office.Interop.Excel;

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
