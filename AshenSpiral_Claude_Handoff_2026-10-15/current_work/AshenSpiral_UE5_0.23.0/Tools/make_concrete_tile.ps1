# Makes the arena concrete tile for 0.25 phase B (Docs/0.25_ASSET_PLAN.md) from the "Concrete column" vault pack, with Windows' own imaging (no extra software).
#   powershell -NoProfile -ExecutionPolicy Bypass -File Tools\make_concrete_tile.ps1
# The pack's textures are UV atlases of a bridge pillar, not tiling surfaces, so this cuts one clean patch of grey concrete out of the 8192 atlas, makes it seamless (the patch is blended with
# itself shifted by half, so the edges come from the middle of the picture) and writes Saved\VaultStage\T_ArenaConcrete.png at 2048. Tools\import_vault_assets.py imports it.
# The tile is a derived work of a licence-gated asset (Gate 0): it stays under Saved\ and is never committed.
param(
    [string]$Vault = 'C:\ProgramData\Epic\EpicGamesLauncher\VaultCache\FabLibrary',
    [string]$Source = 'Concrete_column-315e7849\fbx\concrete-column_extracted\textures\concrete-pillar2.jpeg',
    [int]$CropX = 2200, [int]$CropY = 4900, [int]$CropSize = 1250, [int]$Size = 2048
)
$ErrorActionPreference = 'Stop'
$Project = Split-Path -Parent $PSScriptRoot
$Out = Join-Path $Project 'Saved\VaultStage'
New-Item -ItemType Directory -Force $Out | Out-Null
$In = Join-Path $Vault $Source
if (-not (Test-Path $In)) { throw "Missing $In" }
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
public static class ConcreteTile {
    public static void Make(string input, string output, int cx, int cy, int crop, int size) {
        using (Image source = Image.FromFile(input))
        using (Bitmap tile = new Bitmap(size, size, PixelFormat.Format32bppArgb)) {
            using (Graphics g = Graphics.FromImage(tile)) {
                g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
                g.DrawImage(source, new Rectangle(0, 0, size, size), new Rectangle(cx, cy, crop, crop), GraphicsUnit.Pixel);
            }
            BitmapData data = tile.LockBits(new Rectangle(0, 0, size, size), ImageLockMode.ReadWrite, PixelFormat.Format32bppArgb);
            int stride = data.Stride;
            byte[] src = new byte[stride * size];
            byte[] dst = new byte[stride * size];
            System.Runtime.InteropServices.Marshal.Copy(data.Scan0, src, 0, src.Length);
            int half = size / 2;
            double[] mask = new double[size];
            for (int i = 0; i < size; i++) mask[i] = 0.5 - 0.5 * Math.Cos(2.0 * Math.PI * i / size); // 0 at the edges, 1 in the middle
            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    double w = mask[x] * mask[y];
                    int a = y * stride + x * 4;
                    int b = ((y + half) % size) * stride + ((x + half) % size) * 4;
                    double[] v = new double[3];
                    for (int c = 0; c < 3; c++) v[c] = src[a + c] * w + src[b + c] * (1.0 - w);
                    double lum = 0.114 * v[0] + 0.587 * v[1] + 0.299 * v[2]; // BGRA order; keep a hint of the pack's colour, the game tints it per block
                    for (int c = 0; c < 3; c++) dst[a + c] = (byte)Math.Max(0, Math.Min(255, Math.Round(lum + (v[c] - lum) * 0.35)));
                    dst[a + 3] = 255;
                }
            }
            System.Runtime.InteropServices.Marshal.Copy(dst, 0, data.Scan0, dst.Length);
            tile.UnlockBits(data);
            tile.Save(output, ImageFormat.Png);
        }
    }
}
'@ -ReferencedAssemblies System.Drawing
$Target = Join-Path $Out 'T_ArenaConcrete.png'
[ConcreteTile]::Make($In, $Target, $CropX, $CropY, $CropSize, $Size)
"wrote $Target"
