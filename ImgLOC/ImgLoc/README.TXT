

ImgLoc V2 Available Functions

Name: SearchTile : Returns string in format "#Positions|X1|Y1|X2|Y2|Xn|Yn"
Arguments:  
1.  $sendHBitmap = _GDIPlus_BitmapCreateHBITMAPFromBitmap($hBitmap) Type: IntPtr (handle) Description: SourceImage from memmory.
2.  $FFile = PATH TO FILE (.bmp/.png)  Type: string Description: Tile imge to search in SourceImage
3.  $Tolerance = 0.XX Type: float Description: tolerance to be used in image compare
Usage: $res = DllCall($LibDir & "\ImgLoc.dll", "str", "SearchTile", "handle", $sendHBitmap, "str", $FFile , "float", $Tolerance)

NOTE: Will Always search Inside COCDIamond and CocArea
-------

Name: SearchInSource: Returns string in format "#Positions|X1|Y1|X2|Y2|Xn|Yn"
Arguments:  
1.  $OFile =  PATH TO FILE (.bmp/.png) Type: string Description: SourceImage from file.
2.  $FFile = PATH TO FILE (.bmp/.png)  Type: string Description: Tile imge to search in SourceImage
3.  $Tolerance = 0.XX Type: float Description: tolerance to be used in image compare
Usage: $res = DllCall($LibDir & "\ImgLoc.dll", "str", "SearchInSource", "str", $OFile, "str", $FFile , "float", $Tolerance)
NOTE: Will Always search Inside COCDIamond and CocArea
-------

Name: SearchArea: Returns string in format "#Positions|X1|Y1|X2|Y2|Xn|Yn"
Arguments:  
1.  $sendHBitmap = _GDIPlus_BitmapCreateHBITMAPFromBitmap($hBitmap) Type: IntPtr (handle) Description: SourceImage from memmory.
2.  $FFile = PATH TO FILE (.bmp/.png)  Type: string Description: Tile imge to search in SourceImage
3.  $Tolerance = 0.XX Type: float Description: tolerance to be used in image compare
4.  $X = 100 Type: Int Description: X Start Position for Search Area Defenition
5.  $Y = 100 Type: Int Description: Y Start Position for Search Area Defenition
6.  $W = 250 Type: Int Description: Search Area WIDTH
7.  $H = 250 Type: Int Description: Search Area HEIGTH
Usage:  $res = DllCall($LibDir & "\ImgLoc.dll", "str", "SearchArea", "str", $OFile, "str", $FFile , "float", $Tolerance, "int", $X, "int", $Y, "int", $W, "int", $W )
NOTE: Will Always search Inside COCDIamond And (COCAREA is defined by SearchArea sepcified in the given coordinates)


Current Values for COCDiamond and COCAREA

CocDiamond: Polygon defined with the following coordinates: topP -> rightP -> BottomP -> LeftP -> TopP

TopP = (430, 70)
LeftP = (67, 333)
RightP = (787, 335);
BottomP = (430, 605);

COCArea: Rectangle defined with the following size: (area delimited by the vertices of the building area)
X: 70
Y: 70
WIDHT: 720 
HEIGHT: 540
---------------------------------------------------


27-01-2016

IMGLocV3 

Forget SearchArea Function

Only Function is SearchTile & SearchInSource with the following arguments

SearchTile will also work as a search area...

Name: SearchTile : Returns string in format "#Positions|X1|Y1|X2|Y2|Xn|Yn"
Arguments:  
1.  $sendHBitmap = _GDIPlus_BitmapCreateHBITMAPFromBitmap($hBitmap) Type: IntPtr (handle) Description: SourceImage from memmory.
2.  $FFile = PATH TO FILE (.bmp/.png)  Type: string Description: Tile imge to search in SourceImage
3.  $Tolerance = 0.XX Type: float Description: tolerance to be used in image compare
4.  $SearchArea = "70|70|720|540"  Type: string Description: rectangle in format x|y|Width|Height
5.  $CodDiamond = "430,70|787,335|430,605|67,333" Type: string Description: 4 points that form a poligon top +left + bottom + right in the format x,y|x1,y1|xn,yn...

Usage: $res = DllCall($LibDir & "\ImgLoc.dll", "str", "SearchTile", "handle", $sendHBitmap, "str", $FFile , "float", $Tolerance, "string",$DefaultCocSearchArea "string",$DefaultCocDiamond   )

NOTE: Will Always search Inside COCDIamond and SearchArea (points must be inside intersection)



$DefaultCocDiamond (DEFAULT No GRass ) "430,70|787,335|430,605|67,333"  
$ExtendedCocDiamond (With Grass ) "430,25|840,335|430,645|15,333" 

$DefaultCocSearchArea (Deafault) "70|70|720|540"
$ExtendedCocSearchArea (Extended) "15|25|825|625"
