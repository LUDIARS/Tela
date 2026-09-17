// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
using System;

namespace Tela.Editor
{
    internal static class LocalPipeName
    {
        internal static void Validate(string name)
        {
            if (string.IsNullOrEmpty(name) || name.Length > 128 || name == ".")
                throw new ArgumentException("Enter a local pipe name (1-128 letters, digits, '.', '-' or '_').", nameof(name));
            foreach (var c in name)
                if (!char.IsLetterOrDigit(c) && c != '.' && c != '-' && c != '_')
                    throw new ArgumentException("Pipe names cannot contain paths or remote hosts.", nameof(name));
        }
    }
}
