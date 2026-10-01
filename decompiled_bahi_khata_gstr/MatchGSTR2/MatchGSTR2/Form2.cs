using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Runtime.CompilerServices;
using System.Windows.Forms;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.CompilerServices;

namespace MatchGSTR2;

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
