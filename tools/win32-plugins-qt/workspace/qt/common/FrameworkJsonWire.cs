// Minimal transport API for the Windows .NET Framework build. Algorithms never use this serializer.
#if NETFRAMEWORK
using System;
using System.Collections;
using System.Collections.Generic;
using System.Web.Script.Serialization;
namespace System.Text.Json {
    public static class FrameworkConsole {
        [Runtime.InteropServices.DllImport("kernel32.dll")] static extern IntPtr GetStdHandle(int kind);
        public static void ConfigureInput() {
            var handle = new Microsoft.Win32.SafeHandles.SafeFileHandle(GetStdHandle(-10), false);
            System.IO.FileStream stream;
            try { stream = new System.IO.FileStream(handle, System.IO.FileAccess.Read, 4096, true); }
            catch(ArgumentException) { stream = new System.IO.FileStream(handle, System.IO.FileAccess.Read, 4096, false); }
            Console.SetIn(new System.IO.StreamReader(stream, new UTF8Encoding(false)));
        }
    }
    public sealed class JsonDocument : IDisposable {
        public JsonElement RootElement { get; private set; }
        public static JsonDocument Parse(string text) => new JsonDocument { RootElement = new JsonElement(JsonSerializer.New().DeserializeObject(text)) };
        public void Dispose() { }
    }
    public struct JsonProperty {
        public string Name { get; internal set; }
        public JsonElement Value { get; internal set; }
    }
    public struct JsonElement {
        readonly object value;
        internal JsonElement(object value) { this.value = value; }
        public JsonElement GetProperty(string key) => new JsonElement(((IDictionary<string,object>)value)[key]);
        public bool TryGetProperty(string key, out JsonElement result) {
            if (value is IDictionary<string,object> map && map.TryGetValue(key, out var item)) { result = new JsonElement(item); return true; }
            result = default; return false;
        }
        public string GetString() => (string)value;
        public int GetInt32() => Convert.ToInt32(value, Globalization.CultureInfo.InvariantCulture);
        public bool GetBoolean() => (bool)value;
        public string GetRawText() => JsonSerializer.Serialize(value);
        public IEnumerable<JsonProperty> EnumerateObject() {
            foreach(var pair in (IDictionary<string,object>)value) yield return new JsonProperty { Name = pair.Key, Value = new JsonElement(pair.Value) };
        }
    }
    public static class JsonSerializer {
        internal static JavaScriptSerializer New() => new JavaScriptSerializer { MaxJsonLength = int.MaxValue, RecursionLimit = 256 };
        public static string Serialize(object value) => New().Serialize(value);
        public static object Deserialize(string text, Type type) => New().Deserialize(text, type);
    }
}
#endif
