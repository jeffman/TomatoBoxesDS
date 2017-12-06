VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3180
   ClientLeft      =   60
   ClientTop       =   360
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   ScaleHeight     =   3180
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
Open "J:\desktop\boxes2.map" For Binary As #1

Dim B(0 To &H4A6C&) As Byte
Dim I As Integer
For I = 1 To UBound(B) + 1
    Get #1, I, B(I - 1)
Next I

Close #1

Dim bAddr As Long
bAddr = &HC&

Open "J:\dsdev\tomatoboxes\OUT_NAMES.txt" For Output As #1
Open "J:\dsdev\tomatoboxes\OUT_HIGH.txt" For Output As #2
Open "J:\dsdev\tomatoboxes\OUT_PAR.txt" For Output As #3
Open "J:\dsdev\tomatoboxes\OUT_DATA.txt" For Output As #4
Open "J:\dsdev\tomatoboxes\OUT_TILES.txt" For Output As #5

Dim S1 As String, S2 As String, S3 As String, S4 As String, S5 As String

S1 = "const char mapnames[55][32] = { "
S2 = "const uint16 maphighscore[55] = { "
S3 = "const uint16 mapparscore[55] = { "
S4 = "const uint8 map[55][15][20] = { "
S5 = "const uint16 pal[16] = { "

Dim pal(0 To 15) As Long
pal(0) = RGB(0, 0, 255)
pal(1) = 0
pal(2) = RGB(255, 255, 255)
pal(3) = RGB(125, 0, 0)
pal(4) = RGB(195, 195, 195)
pal(5) = RGB(231, 231, 231)
pal(6) = RGB(0, 255, 0)
pal(7) = RGB(255, 0, 0)
pal(8) = RGB(0, 130, 0)

Dim R As Byte, G As Byte, Bb As Byte, tmp As Long

For I = 0 To 15
    R = Int((pal(I) And &HFF&) / 8)
    G = Int(((pal(I) And &HFF00&) / &H100&) / 8)
    Bb = Int(((pal(I) And &HFF0000) / &H10000) / 8)
    tmp = R + (CLng(G) * 32) + (CLng(Bb) * 1024)
    S5 = S5 & "0x" & Right("0000" & Hex(tmp), 4) & ", "
Next I
S5 = S5 & " };"

Dim J As Integer, K As Integer, L As Integer
L = &H154&

For I = 0 To 54
    
    'Names
    S1 = S1 & "{ "
    For J = 0 To 31
        If B(bAddr + (I * L) + J) = 0 Then Exit For
        S1 = S1 & "'" & IIf(Chr(B(bAddr + (I * L) + J)) = "'", "\'", Chr(B(bAddr + (I * L) + J))) & IIf(B(bAddr + (I * L) + J + 1) = 0, "'", "', ")
    Next J
    S1 = S1 & " }," & vbNewLine & "                          "
    
    'High scores
    S2 = S2 & CStr(B(bAddr + 32 + (I * L)) + (CLng(B(bAddr + 32 + 1 + (I * L)) * 256))) & ", "
    
    'Par scores
    S3 = S3 & CStr(B(bAddr + 36 + (I * L)) + (CLng(B(bAddr + 36 + 1 + (I * L)) * 256))) & ", "
    
    'Map data
    S4 = S4 & "{ "
    For J = 0 To 14
        S4 = S4 & "{ "
        For K = 0 To 19
            S4 = S4 & B(bAddr + 40 + (I * L) + (J * 20) + K) & IIf(K = 19, "", ", ")
            Select Case B(bAddr + 40 + (I * L) + (J * 20) + K)
            Case 0, 1, 2, 3, 4, 5, 8
            Case Else
                Debug.Print B
            End Select
        Next K
        S4 = S4 & " }" & IIf(J = 14, "", ",") & vbNewLine & "                          "
    Next J
    S4 = S4 & " }," & vbNewLine & "                          "
    
Next I
    
S1 = S1 & " };"
S2 = S2 & " };"
S3 = S3 & " };"
S4 = S4 & " };"

Print #1, S1
Print #2, S2
Print #3, S3
Print #4, S4
Print #5, S5

Reset

End Sub
