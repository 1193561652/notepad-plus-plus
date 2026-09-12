"""Compile original managed plugin cores against the installed Windows Framework.
Uses SDK Roslyn for modern C# syntax, and no NuGet packages or global changes.
"""
import argparse, glob, os, pathlib, re, subprocess, xml.etree.ElementTree as ET
p = argparse.ArgumentParser()
p.add_argument("--dotnet", required=True)
p.add_argument("--project", type=pathlib.Path, required=True)
p.add_argument("--output", type=pathlib.Path, required=True)
a = p.parse_args()
a.project = a.project.resolve(); a.output = a.output.resolve()
sdks = subprocess.check_output([a.dotnet, "--list-sdks"], text=True)
entries = re.findall(r"^(\S+) \[(.+)\]$", sdks, re.M)
if not entries: raise SystemExit("The .NET SDK with Roslyn is required")
version, directory = entries[-1]
csc = pathlib.Path(directory) / version / "Roslyn/bincore/csc.dll"
framework = pathlib.Path(os.environ["WINDIR"]) / "Microsoft.NET/Framework64/v4.0.30319"
if not framework.exists(): framework = pathlib.Path(os.environ["WINDIR"]) / "Microsoft.NET/Framework/v4.0.30319"
refs = ["mscorlib.dll", "System.dll", "System.Core.dll", "System.Xml.dll", "System.Net.Http.dll", "System.Web.Extensions.dll", "System.Web.dll", "System.Drawing.dll"]
sources = []
for compile in ET.parse(a.project).iter("Compile"):
    excludes = set()
    for pattern in compile.attrib.get("Exclude", "").split(";"):
        if pattern: excludes.update(pathlib.Path(x).resolve() for x in glob.glob(str(a.project.parent / pattern)))
    sources.extend(pathlib.Path(x).resolve() for x in glob.glob(str(a.project.parent / compile.attrib["Include"])) if pathlib.Path(x).resolve() not in excludes)
sources.append(pathlib.Path(__file__).with_name("FrameworkJsonWire.cs").resolve())
a.output.parent.mkdir(parents=True, exist_ok=True)
args = ["/nologo", "/target:exe", "/langversion:latest", "/unsafe", "/define:NPP_QT_PORT,NETFRAMEWORK", '/out:"' + str(a.output) + '"']
args += ['/reference:"' + str(framework / x) + '"' for x in refs]
args += ['"' + str(x) + '"' for x in sources]
rsp = a.output.with_suffix(".rsp")
rsp.write_text("\n".join(args), encoding="utf-8")
subprocess.run([a.dotnet, str(csc), "@" + str(rsp)], check=True)
a.output.with_suffix(".exe.config").write_text('<configuration><startup><supportedRuntime version="v4.0" sku=".NETFramework,Version=v4.8" /></startup></configuration>', encoding="utf-8")
