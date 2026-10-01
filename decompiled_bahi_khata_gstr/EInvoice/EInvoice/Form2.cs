using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.CompilerServices;
using System.Windows.Forms;
using Microsoft.VisualBasic;
using Microsoft.VisualBasic.CompilerServices;
using Newtonsoft.Json;

namespace EInvoice;

[DesignerGenerated]
public class Form2 : Form
{
	private IContainer components;

	private string JpgImageName;

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

	[field: AccessedThroughProperty("TextBox1")]
	internal virtual TextBox TextBox1
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

	[field: AccessedThroughProperty("PictureBox1")]
	internal virtual PictureBox PictureBox1
	{
		get;
		[MethodImpl(MethodImplOptions.Synchronized)]
		set;
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
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_000c: Expected Obj, but got Unknown
		//IL_000e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0018: Expected Obj, but got Unknown
		//IL_001a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0024: Expected Obj, but got Unknown
		//IL_0026: Unknown result type (might be due to invalid IL or missing references)
		//IL_0030: Expected Obj, but got Unknown
		Button1 = new Button();
		TextBox1 = new TextBox();
		Button2 = new Button();
		PictureBox1 = new PictureBox();
		((ISupportInitialize)PictureBox1).BeginInit();
		((Control)this).SuspendLayout();
		((Control)Button1).Location = new Point(587, 320);
		((Control)Button1).Name = "Button1";
		((Control)Button1).Size = new Size(152, 62);
		((Control)Button1).TabIndex = 0;
		((ButtonBase)Button1).Text = "Button1";
		((ButtonBase)Button1).UseVisualStyleBackColor = true;
		((Control)Button1).Visible = false;
		((Control)TextBox1).Location = new Point(26, 29);
		TextBox1.Multiline = true;
		((Control)TextBox1).Name = "TextBox1";
		((Control)TextBox1).Size = new Size(483, 215);
		((Control)TextBox1).TabIndex = 1;
		((Control)TextBox1).Visible = false;
		((Control)Button2).Location = new Point(625, 200);
		((Control)Button2).Name = "Button2";
		((Control)Button2).Size = new Size(97, 43);
		((Control)Button2).TabIndex = 2;
		((ButtonBase)Button2).Text = "Button2";
		((ButtonBase)Button2).UseVisualStyleBackColor = true;
		((Control)Button2).Visible = false;
		PictureBox1.BorderStyle = (BorderStyle)1;
		((Control)PictureBox1).Location = new Point(599, 29);
		((Control)PictureBox1).Name = "PictureBox1";
		((Control)PictureBox1).Size = new Size(123, 112);
		PictureBox1.SizeMode = (PictureBoxSizeMode)1;
		PictureBox1.TabIndex = 4;
		PictureBox1.TabStop = false;
		((Control)PictureBox1).Visible = false;
		((ContainerControl)this).AutoScaleDimensions = new SizeF(6f, 13f);
		((ContainerControl)this).AutoScaleMode = (AutoScaleMode)1;
		((Form)this).ClientSize = new Size(800, 450);
		((Control)this).Controls.Add((Control)(object)PictureBox1);
		((Control)this).Controls.Add((Control)(object)Button2);
		((Control)this).Controls.Add((Control)(object)TextBox1);
		((Control)this).Controls.Add((Control)(object)Button1);
		((Control)this).Name = "Form2";
		((Form)this).Text = "Form2";
		((ISupportInitialize)PictureBox1).EndInit();
		((Control)this).ResumeLayout(false);
		((Control)this).PerformLayout();
	}

	private void Button1_Click(object sender, EventArgs e)
	{
		Dictionary<string, object> dictionary = JsonConvert.DeserializeObject<Dictionary<string, object>>(TextBox1.Text);
		string text = dictionary["IRN"].ToString();
		string prompt = dictionary["SignedQRCode"].ToString();
		Interaction.MsgBox(prompt);
	}

	private Bitmap GetImageFromBase64(object Base64String)
	{
		//IL_0030: Unknown result type (might be due to invalid IL or missing references)
		//IL_0036: Expected Obj, but got Unknown
		Bitmap val = default;
		if (Operators.ConditionalCompareObjectNotEqual(string.Empty, Base64String, TextCompare: false))
		{
			byte[] buffer = Convert.FromBase64String(Conversions.ToString(Base64String));
			using MemoryStream memoryStream = new MemoryStream(buffer);
			val = (Bitmap)Image.FromStream((Stream)memoryStream);
			if (!Information.IsNothing(val))
			{
				if (!Directory.Exists("c:\\Base64ImageViwer"))
				{
					Directory.CreateDirectory("c:\\Base64ImageViwer");
				}
				((Image)val).Save("c:\\Base64ImageViwer\\TempImg.jpg", ImageFormat.Jpeg);
				((Image)val).Save(JpgImageName, ImageFormat.Jpeg);
				PictureBox1.Image = (Image)(object)val;
			}
		}
		return val;
	}

	private void Button2_Click(object sender, EventArgs e)
	{
		JpgImageName = Application.StartupPath + "\\IRN.jpg";
		GetImageFromBase64(TextBox1.Text);
	}

	private void Form2_Load(object sender, EventArgs e)
	{
		Interaction.MsgBox("Form 2");
	}
}
