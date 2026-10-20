# 0.24.2 aesthetics: brings the textures exported by Tools/import_vault_assets.py phase G (Saved\VaultStage\DC\*.png, the Dragon Cave pack's 4096-8192 sources) down to at most
# 2048 a side, high-quality bicubic, into Saved\VaultStage\DC\small, for phase H to import. Normal maps are renormalised after the resample. Windows' own imaging; nothing to install.
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tools\shrink_vault_textures.ps1
param([int]$MaxSize=2048,[switch]$Force) # -Force redoes textures already shrunk
$ErrorActionPreference='Stop'
$Project=Split-Path -Parent $PSScriptRoot
$In=Join-Path $Project 'Saved\VaultStage\DC';$Out=Join-Path $In 'small'
New-Item -ItemType Directory -Force $Out | Out-Null
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;using System.Drawing;using System.Drawing.Imaging;using System.Runtime.InteropServices;
public static class AshenNormals {
    // Each pixel's RGB is read as a vector in -1..1 and scaled back to unit length (blue up); averaging in the resample shortens them.
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
foreach($File in Get-ChildItem $In -Filter *.png) {
    $Target=Join-Path $Out $File.Name
    if(-not $Force -and (Test-Path $Target) -and (Get-Item $Target).LastWriteTime -ge $File.LastWriteTime){continue}
    $Src=[System.Drawing.Image]::FromFile($File.FullName)
    $Scale=[Math]::Min(1.0,$MaxSize/[double][Math]::Max($Src.Width,$Src.Height))
    $W=[int][Math]::Round($Src.Width*$Scale);$H=[int][Math]::Round($Src.Height*$Scale)
    $Dst=New-Object System.Drawing.Bitmap $W,$H,([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $G=[System.Drawing.Graphics]::FromImage($Dst);$G.InterpolationMode='HighQualityBicubic';$G.PixelOffsetMode='HighQuality';$G.CompositingMode='SourceCopy'
    $Wrap=New-Object System.Drawing.Imaging.ImageAttributes;$Wrap.SetWrapMode([System.Drawing.Drawing2D.WrapMode]::TileFlipXY) # no dark seams at the edges of a tiling texture
    $G.DrawImage($Src,(New-Object System.Drawing.Rectangle 0,0,$W,$H),0,0,$Src.Width,$Src.Height,[System.Drawing.GraphicsUnit]::Pixel,$Wrap);$G.Dispose();$Src.Dispose()
    if($File.BaseName -match '_N$'){[AshenNormals]::Renormalise($Dst)}
    $Dst.Save((Join-Path $Out $File.Name),[System.Drawing.Imaging.ImageFormat]::Png);$Dst.Dispose()
    Write-Output ("{0}: {1}x{2}" -f $File.Name,$W,$H)
}
