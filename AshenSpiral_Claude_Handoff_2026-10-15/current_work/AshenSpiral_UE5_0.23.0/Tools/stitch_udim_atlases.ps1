# 0.24.2 aesthetics: the Dragon Cave pack's UDIM texture sets (several 4096 tiles named <texture>.1001.png, .1002.png ... as exported by Tools/import_vault_assets.py phase G) become one
# atlas each, so the game can draw them without virtual texturing: tile t (t = u + 4v for UDIM 1001 + u + 10v) goes to cell (t mod Cols, t div Cols) of a Cols x Rows grid, every cell the
# same size, the whole atlas at most 4096 wide. M_DecorAtlas (phase J) undoes the mapping per pixel. Normal maps are renormalised after the resample. Windows' own imaging.
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tools\stitch_udim_atlases.ps1
param([int]$AtlasWidth=4096,[switch]$Force)
$ErrorActionPreference='Stop'
$Project=Split-Path -Parent $PSScriptRoot
$In=Join-Path $Project 'Saved\VaultStage\DC';$Out=Join-Path $In 'small'
New-Item -ItemType Directory -Force $Out | Out-Null
Add-Type -AssemblyName System.Drawing
if(-not ('AshenAtlasNormals' -as [type])) {
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;using System.Drawing;using System.Drawing.Imaging;using System.Runtime.InteropServices;
public static class AshenAtlasNormals {
    public static void Renormalise(Bitmap b) {
        var r=new Rectangle(0,0,b.Width,b.Height);var d=b.LockBits(r,ImageLockMode.ReadWrite,PixelFormat.Format32bppArgb);
        int n=d.Stride*b.Height;var px=new byte[n];Marshal.Copy(d.Scan0,px,0,n);
        for(int i=0;i<n;i+=4) {
            double x=px[i+2]/127.5-1,y=px[i+1]/127.5-1,z=px[i]/127.5-1;double l=Math.Sqrt(x*x+y*y+z*z);if(l<1e-6){x=0;y=0;z=1;l=1;}
            px[i+2]=(byte)Math.Round((x/l+1)*127.5);px[i+1]=(byte)Math.Round((y/l+1)*127.5);px[i]=(byte)Math.Round((z/l+1)*127.5);
        }
        Marshal.Copy(px,0,d.Scan0,n);b.UnlockBits(d);
    }
}
'@
}
# set stem -> grid columns, rows (the tiles' UDIM numbers decide which cell each goes to); keep in step with DECOR_ATLASES in Tools/import_vault_assets.py
$Sets=[ordered]@{ 'T_BridgeSet'=@(4,2); 'T_InfernoRocks01'=@(2,2); 'T_LargePillars'=@(2,1); 'T_StoneWall'=@(2,2); 'T_Stoneworks'=@(4,2) }
foreach($Stem in $Sets.Keys) {
    $Cols=$Sets[$Stem][0];$Rows=$Sets[$Stem][1];$Cell=[int]($AtlasWidth/$Cols);$W=$Cell*$Cols;$H=$Cell*$Rows
    foreach($Map in @('BC','N','ORM')) {
        $Target=Join-Path $Out ("{0}_{1}_Atlas.png" -f $Stem,$Map)
        $Tiles=Get-ChildItem $In -Filter ("{0}_{1}.1*.png" -f $Stem,$Map) | Sort-Object Name
        if($Tiles.Count -eq 0){throw "No UDIM tiles for ${Stem}_$Map in $In (run import_vault_assets.py phase G)"}
        if(-not $Force -and (Test-Path $Target) -and (Get-Item $Target).LastWriteTime -ge ($Tiles | Measure-Object -Property LastWriteTime -Maximum).Maximum){continue}
        $Atlas=New-Object System.Drawing.Bitmap $W,$H,([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $G=[System.Drawing.Graphics]::FromImage($Atlas);$G.InterpolationMode='HighQualityBicubic';$G.PixelOffsetMode='HighQuality';$G.CompositingMode='SourceCopy'
        $Wrap=New-Object System.Drawing.Imaging.ImageAttributes;$Wrap.SetWrapMode([System.Drawing.Drawing2D.WrapMode]::TileFlipXY)
        foreach($Tile in $Tiles) {
            $Udim=[int]($Tile.BaseName.Split('.')[-1]);$Index=$Udim-1001;$U=$Index%10;$V=[Math]::Floor($Index/10);$T=$U+4*$V
            if($T -ge $Cols*$Rows){throw "$($Tile.Name): tile $T does not fit a ${Cols}x$Rows grid"}
            $X=($T%$Cols)*$Cell;$Y=[Math]::Floor($T/$Cols)*$Cell # Unreal's V grows downward like the image rows, and UDIM 1011 sits below 1001: rows go top to bottom
            $Src=[System.Drawing.Image]::FromFile($Tile.FullName)
            $G.DrawImage($Src,(New-Object System.Drawing.Rectangle $X,$Y,$Cell,$Cell),0,0,$Src.Width,$Src.Height,[System.Drawing.GraphicsUnit]::Pixel,$Wrap);$Src.Dispose()
        }
        $G.Dispose()
        if($Map -eq 'N'){[AshenAtlasNormals]::Renormalise($Atlas)}
        $Atlas.Save($Target,[System.Drawing.Imaging.ImageFormat]::Png);$Atlas.Dispose()
        Write-Output ("{0}: {1} tiles into {2}x{3} ({4}x{5} cells of {6})" -f (Split-Path $Target -Leaf),$Tiles.Count,$W,$H,$Cols,$Rows,$Cell)
    }
}
