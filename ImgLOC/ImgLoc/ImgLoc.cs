using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using AForge.Imaging;
using System.Runtime.InteropServices;
using RGiesecke.DllExport;


namespace ImgLoc
{
    [ComVisible(true)]
    public class ImgLoc
    {

        /*
        [DllExport("SearchTile", CallingConvention = CallingConvention.StdCall)]
       // [return: MarshalAs(UnmanagedType.LPStr)]
        public static string SearchTileInImage(IntPtr source, string tile, float similarity)
        {
            try
            {
                //float sim = float.Parse(similarity);
                Bitmap bitmap1 = System.Drawing.Image.FromHbitmap(source);
                Bitmap bitmap2 = (Bitmap)System.Drawing.Image.FromFile(tile);
                Bitmap image = ImgLoc.ConvertToFormat((System.Drawing.Image)bitmap1, PixelFormat.Format24bppRgb);
                Bitmap template = ImgLoc.ConvertToFormat((System.Drawing.Image)bitmap2, PixelFormat.Format24bppRgb);
                Rectangle r = new Rectangle(70, 70, 720, 540);
                TemplateMatch[] templateMatchArray = new ExhaustiveTemplateMatching(similarity).ProcessImage(image, template,r);
                string str = templateMatchArray.Length.ToString();
                foreach (TemplateMatch templateMatch in templateMatchArray)
                    str = str + "|" + (templateMatch.Rectangle.X + template.Width / 2).ToString() + "|" + (templateMatch.Rectangle.Y + template.Height / 2).ToString();

                bitmap1.Dispose();
                bitmap2.Dispose();
                image.Dispose();
                template.Dispose();
                return str;
            }
            catch
            {
                return "-1";
            }
        }
        */
        [DllExport("SearchTile", CallingConvention = CallingConvention.StdCall)]
        // [return: MarshalAs(UnmanagedType.LPStr)]
        public static string SearchTileInImage(IntPtr source, string tile, float similarity, string cocarea = "70|70|720|540",string cocDiamondpoints = "430,70|787,335|430,605|67,333")
        {
            try
            {
                string[] pointlist = cocDiamondpoints.Split('|');
                PointF topP = new PointF(float.Parse(pointlist[0].Split(',')[0]), float.Parse(pointlist[0].Split(',')[1]));
                PointF RightP = new PointF(float.Parse(pointlist[1].Split(',')[0]), float.Parse(pointlist[1].Split(',')[1]));
                PointF BottomP = new PointF(float.Parse(pointlist[2].Split(',')[0]), float.Parse(pointlist[2].Split(',')[1]));
                PointF LeftP = new PointF(float.Parse(pointlist[3].Split(',')[0]), float.Parse(pointlist[3].Split(',')[1]));
                List<PointF> m_Points = new List<PointF>();
                m_Points.Add(topP);
                m_Points.Add(RightP);
                m_Points.Add(BottomP);
                m_Points.Add(LeftP);
                m_Points.Add(topP);


                string[] rectarea = cocarea.Split('|');
                Rectangle r = new Rectangle(int.Parse(rectarea[0]), int.Parse(rectarea[1]), int.Parse(rectarea[2]), int.Parse(rectarea[3]));



                Bitmap bitmap1 = System.Drawing.Image.FromHbitmap(source);
                Bitmap bitmap2 = (Bitmap)System.Drawing.Image.FromFile(tile);
                Bitmap image = ImgLoc.ConvertToFormat((System.Drawing.Image)bitmap1, PixelFormat.Format24bppRgb);
                Bitmap template = ImgLoc.ConvertToFormat((System.Drawing.Image)bitmap2, PixelFormat.Format24bppRgb);
                
                TemplateMatch[] templateMatchArray = new ExhaustiveTemplateMatching(similarity).ProcessImage(image, template, r,m_Points);
                string str = templateMatchArray.Length.ToString();
                foreach (TemplateMatch templateMatch in templateMatchArray)
                    str = str + "|" + (templateMatch.Rectangle.X + template.Width / 2).ToString() + "|" + (templateMatch.Rectangle.Y + template.Height / 2).ToString();

                bitmap1.Dispose();
                bitmap2.Dispose();
                image.Dispose();
                template.Dispose();
                return str;
            }
            catch
            {
                return "-1";
            }
        }

        [DllExport("SearchInSource", CallingConvention = CallingConvention.StdCall)]
      //  [return: MarshalAs(UnmanagedType.LPStr)]
        public static string SearchSourceInImage(string source, string tile, float similarity,string cocarea= "70|70|720|540", string cocDiamondpoints = "430,70|787,335|430,605|67,333")
        {
            try
            {
                string[] pointlist = cocDiamondpoints.Split('|');
                PointF topP = new PointF(float.Parse(pointlist[0].Split(',')[0]), float.Parse(pointlist[0].Split(',')[1]));
                PointF RightP = new PointF(float.Parse(pointlist[1].Split(',')[0]), float.Parse(pointlist[1].Split(',')[1]));
                PointF BottomP = new PointF(float.Parse(pointlist[2].Split(',')[0]), float.Parse(pointlist[2].Split(',')[1]));
                PointF LeftP = new PointF(float.Parse(pointlist[3].Split(',')[0]), float.Parse(pointlist[3].Split(',')[1]));
                List<PointF> m_Points = new List<PointF>();
                m_Points.Add(topP);
                m_Points.Add(RightP);
                m_Points.Add(BottomP);
                m_Points.Add(LeftP);
                m_Points.Add(topP);


                string[] rectarea = cocarea.Split('|');
                Rectangle r = new Rectangle(int.Parse(rectarea[0]), int.Parse(rectarea[1]), int.Parse(rectarea[2]), int.Parse(rectarea[3]));

                Bitmap bitmap1 = (Bitmap)System.Drawing.Image.FromFile(source);
                Bitmap bitmap2 = (Bitmap)System.Drawing.Image.FromFile(tile);
                Bitmap image = ImgLoc.ConvertToFormat((System.Drawing.Image)bitmap1, PixelFormat.Format24bppRgb);
                Bitmap template = ImgLoc.ConvertToFormat((System.Drawing.Image)bitmap2, PixelFormat.Format24bppRgb);
                //Rectangle r = new Rectangle(70, 70, 720, 540);
                TemplateMatch[] templateMatchArray = new ExhaustiveTemplateMatching(similarity).ProcessImage(image, template, r, m_Points);

                string str = templateMatchArray.Length.ToString();
                foreach (TemplateMatch templateMatch in templateMatchArray)
                    str = str + "|" + (templateMatch.Rectangle.X + template.Width / 2).ToString() + "|" + (templateMatch.Rectangle.Y + template.Height / 2).ToString();
                bitmap1.Dispose();
                bitmap2.Dispose();
                image.Dispose();
                template.Dispose();
                return str;
            }
            catch
            {
                return "-1";
            }
        }

        [DllExport("MBRSearchImage", CallingConvention = CallingConvention.StdCall)]
        // [return: MarshalAs(UnmanagedType.LPStr)]
        // Pointer without any Rectangle and Polygon
        public static string SearchInImage(IntPtr source, string tile, float similarity)
        {
            try
            {
                Bitmap bitmap1 = System.Drawing.Image.FromHbitmap(source);
                Bitmap bitmap2 = (Bitmap)System.Drawing.Image.FromFile(tile);
                Bitmap image = ImgLoc.ConvertToFormat((System.Drawing.Image)bitmap1, PixelFormat.Format24bppRgb);
                Bitmap template = ImgLoc.ConvertToFormat((System.Drawing.Image)bitmap2, PixelFormat.Format24bppRgb);
                
                TemplateMatch[] templateMatchArray = new ProMacTemplateMatching(similarity).ProcessImage(image, template);
                string str = templateMatchArray.Length.ToString();
                foreach (TemplateMatch templateMatch in templateMatchArray)
                    str = str + "|" + (templateMatch.Rectangle.X + template.Width / 2).ToString() + "|" + (templateMatch.Rectangle.Y + template.Height / 2).ToString();

                bitmap1.Dispose();
                bitmap2.Dispose();
                image.Dispose();
                template.Dispose();
                return str;
            }
            catch
            {
                return "-1";
            }
        }

        public static Bitmap ConvertToFormat(System.Drawing.Image image, PixelFormat format)
        {
            Bitmap bitmap = new Bitmap(image.Width, image.Height, format);
            using (Graphics graphics = Graphics.FromImage(bitmap))
                graphics.DrawImage(image, new Rectangle(0, 0, bitmap.Width, bitmap.Height));
            return bitmap;
        }
    }
}
