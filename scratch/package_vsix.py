"""Packages the vscode-enlang folder into a standard valid VSIX package."""

import os
import zipfile
import json
import xml.sax.saxutils as saxutils

VSCODE_DIR = os.path.abspath("vscode-enlang")

with open(os.path.join(VSCODE_DIR, "package.json"), "r", encoding="utf-8") as f:
    pkg = json.load(f)

OUT_VSIX = os.path.join(VSCODE_DIR, f"enlang-{pkg['version']}.vsix")

name_esc = saxutils.escape(str(pkg.get('name', 'enlang')))
version_esc = saxutils.escape(str(pkg.get('version', '1.0.2')))
publisher_esc = saxutils.escape(str(pkg.get('publisher', 'Enlang1234')))
display_name_esc = saxutils.escape(str(pkg.get('displayName', 'EnLang')))
description_esc = saxutils.escape(str(pkg.get('description', 'Official VS Code extension for EnLang')))

content_types_xml = """<?xml version="1.0" encoding="utf-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="json" ContentType="application/json"/>
  <Default Extension="js" ContentType="application/javascript"/>
  <Default Extension="md" ContentType="text/markdown"/>
  <Default Extension="code-snippets" ContentType="application/json"/>
  <Default Extension="vsixmanifest" ContentType="text/xml"/>
  <Override PartName="/extension.vsixmanifest" ContentType="text/xml"/>
</Types>"""

vsix_manifest_xml = f"""<?xml version="1.0" encoding="utf-8"?>
<PackageManifest Version="2.0.0" xmlns="http://schemas.microsoft.com/developer/vsx-schema/2011" xmlns:d="http://schemas.microsoft.com/developer/vsx-schema-design/2011">
  <Metadata>
    <Identity Id="{name_esc}" Version="{version_esc}" Publisher="{publisher_esc}" Language="en-US"/>
    <DisplayName>{display_name_esc}</DisplayName>
    <Description xml:space="preserve">{description_esc}</Description>
    <Categories>Programming Languages,Snippets,Linters,Formatters,AI</Categories>
  </Metadata>
  <Installation>
    <InstallationTarget Id="Microsoft.VisualStudio.Code"/>
  </Installation>
  <Dependencies/>
  <Assets>
    <Asset Type="Microsoft.VisualStudio.Code.Manifest" Path="extension/package.json" Addressable="true"/>
    <Asset Type="Microsoft.VisualStudio.Services.Content.Details" Path="extension/README.md" Addressable="true"/>
  </Assets>
</PackageManifest>"""

# Files to include in extension/
files_to_pack = [
    "package.json",
    "extension.js",
    "language-configuration.json",
    "README.md",
    "syntaxes/enlang.tmLanguage.json",
    "syntaxes/enlangf.tmLanguage.json",
    "syntaxes/enlangd.tmLanguage.json",
    "syntaxes/enlgs.tmLanguage.json",
    "syntaxes/enlgm.tmLanguage.json",
    "syntaxes/enlgdb.tmLanguage.json",
    "snippets/enlang.code-snippets"
]

with zipfile.ZipFile(OUT_VSIX, "w", zipfile.ZIP_DEFLATED) as z:
    # 1. Content Types & Manifest at root (encoded as UTF-8)
    z.writestr("[Content_Types].xml", content_types_xml.encode("utf-8"))
    z.writestr("extension.vsixmanifest", vsix_manifest_xml.encode("utf-8"))
    
    # 2. Extension files
    for rel_path in files_to_pack:
        full_path = os.path.join(VSCODE_DIR, rel_path)
        if os.path.exists(full_path):
            z.write(full_path, f"extension/{rel_path}")

print(f"Successfully packaged: {OUT_VSIX} ({os.path.getsize(OUT_VSIX)} bytes)")
