using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Microsoft.Office.Interop.Excel;

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
