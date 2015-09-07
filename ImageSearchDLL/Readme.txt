ImageSearch for AutoIT
========================

A DLL library to allow search for an image within the desktop. Also included 
are a AU3 library wrapper, and a sample AU3 program to get you going. All the 
code here is licensed under GPLv2, as it based on the AutoHotKey. See 
License.txt for info.

The DLL was built from a need to do searching a for an image within the desktop. 
I support an ERP application which uses plenty of Java Applets, and needed scripting
automation for mass upload of data. The only way to detect that a record has 
been properly updated was to check for a "success" image. The PixelSearch feature 
was useful, but a pain to program for this purpose. At first I found an image 
library from www.mjtnet.com which did the job, but was slow (and of uncertain copyright). 

AutoHotKey had the function, but I'm a fan of AutoIT, and too lazy to learn a 
new scripting syntax. Seeing that AutoHotKey was opensourced, I took the chance 
to strip out the necessary code and compile it into a DLL using VC++ 2005 
express. The whole exercise was particularly challenging since I've never 
done VC++ before, being more a Java person.

Anyway, here it is. The source code is awful, since I just needed something 
working, and just cut and pasted all the necessary parts together. Hope it 
works for you, since the redistribution of VC++ express applications seems 
to be quite challenging, and I could have got it wrong.


Instructions
================
. Copy ImageSearchDLL.dll to the windows directory (or any path reachable directory)
. Copy ImageSearch.au3 to \AutoIt3\include\ directory
. Give the ImageSearchDemo.au3 a spin. Good luck!


Credits
===============
. AutoHotKey for the source code - Which this DLL is based on
. Aurelian Maga and Chris Mallett for the original code
. The folks at AutoHotKey forum for instructions on how to install and compile AHK with VC++


________________________________
Live long and prosper,
kangkeng 2008
