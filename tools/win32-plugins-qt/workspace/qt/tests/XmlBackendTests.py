"""Exercise the original MSXML implementation through its transport boundary."""
import json
import pathlib
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    schema = root / "schema.xsd"
    schema.write_text('<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema"><xs:element name="n" type="xs:integer"/></xs:schema>', encoding="utf-8")
    style = root / "style.xsl"
    style.write_text('<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"><xsl:output method="text" encoding="UTF-8"/><xsl:template match="/"><xsl:value-of select="root/x"/></xsl:template></xsl:stylesheet>', encoding="utf-8")
    requests = [
        {"op": "syntax", "text": "<root/>"},
        {"op": "syntax", "text": "<root>"},
        {"op": "validate", "text": "<n>123</n>", "schema": str(schema)},
        {"op": "validate", "text": "<n>bad</n>", "schema": str(schema)},
        {"op": "xpath", "text": '<root xmlns:p="urn:test"><p:x>值</p:x></root>', "xpath": "//p:x", "namespaces": "xmlns:p='urn:test'"},
        {"op": "xslt", "text": "<root><x>转换值</x></root>", "stylesheet": str(style)},
    ]
    result = subprocess.run([sys.argv[1]], input="".join(json.dumps(r, ensure_ascii=False) + "\n" for r in requests), encoding="utf-8", capture_output=True, timeout=30, check=True)
    responses = [json.loads(line) for line in result.stdout.splitlines()]
    assert len(responses) == len(requests), result.stdout
    assert all(r["ok"] for r in responses), responses
    values = [r["result"] for r in responses]
    assert values[0]["valid"]
    assert not values[1]["valid"] and values[1]["errors"]
    assert values[2]["valid"], values[2]
    assert not values[3]["valid"] and values[3]["errors"]
    assert values[4]["nodes"][0]["value"] == "值", values[4]
    assert values[5]["valid"] and values[5]["text"] == "转换值", values[5]
    print("Original MSXML syntax, XSD, namespace XPath and UTF-8 XSLT passed")
