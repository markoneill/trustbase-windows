using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace PluginManager
{
    // Self-Contained Variables //
    public partial class ConfigGramer
    {
        private _Configuration _BaseConfiguration;
        private List<_Token> _Tokens;

        // REGEX to identify Comments //
        private Regex singleLineCommentRegex = new Regex(@"(\/\/|\#).*");
        private Regex multiLineCommentRegex = new Regex(@"/\*[^*]*\*+(?:[^/*][^*]*\*+)*/");

        // REGEX to identify Terminals //
        private Regex whiteSpaceRegex = new Regex(@"[^\s\:\=\;\,\[\]\(\)\{\}]+");
        private Regex stringRegex = new Regex(@"""([^""\\]|\\.)*""");
        private Regex booleanRegex = new Regex(@"([Tt][Rr][Uu][Ee])|([Ff][Aa][Ll][Ss][Ee])");
        private Regex nameRegex = new Regex(@"[A-Za-z\*][-A-Za-z0-9_\*]*");
        private Regex octalRegex = new Regex(@"[-+]?0[0-9]*[1-9][0-9]*");
        private Regex octal64Regex = new Regex(@"[-+]?0[0-9]*[1-9][0-9]*L(L)?");
        private Regex integerRegex = new Regex(@"[-+]?[0-9]+");
        private Regex integer64Regex = new Regex(@"[-+]?[0-9]+L(L)?");
        private Regex hexRegex = new Regex(@"0[Xx][0-9A-Fa-f]+");
        private Regex hex64Regex = new Regex(@"0[Xx][0-9A-Fa-f]+L(L)?");
        private Regex floatRegex = new Regex(@"([-+]?([0-9]*)?\.[0-9]*([eE][-+]?[0-9]+)?)|([-+]([0-9]+)(\.[0-9]*)?[eE][-+]?[0-9]+)");
    }

    // Contains all of the Models for the Configuration Manager //
    public partial class ConfigGramer
    {
        public abstract class _Expression { }
        public abstract class _Terminal : _Expression
        {
            public TerminalType Type { get; set; }
        }
        public abstract class _TerminalExpression<T> : _Terminal
        {
            public T Value { get; set; }

            protected _TerminalExpression(TerminalType type, T value)
            {
                Type = type;
                Value = value;
            }
        }
        public abstract class _NonTerminal : _Expression { }
        public abstract class _ValueOption : _NonTerminal { }

        // Terminal Values //
        /**
         * 
         * In C and C++, integer, 64-bit integer, floating point, and string 
         * values are mapped to the native types int, long long, double, and 
         * const char *, respectively. The boolean type is mapped to int in C 
         * and bool in C++.
         * 
         * integer -> int
         * 64-bit integer -> long long (c# = long)
         * floating point -> double
         * string -> const char * (c# = string)
         * 
         * */
        public class _Boolean : _TerminalExpression<bool>
        {
            public _Boolean(bool value) : base(TerminalType.BOOLEAN, value) { }
        }

        public class _Integer : _TerminalExpression<int>
        {
            public _Integer(int value) : base(TerminalType.INTEGER, value) { }
            protected _Integer(TerminalType type, int value) : base(type, value) { }
        }

        public class _Integer64 : _TerminalExpression<long>
        {
            public _Integer64(long value) : base(TerminalType.INTEGER64, value) { }
            protected _Integer64(TerminalType type, long value) : base(type, value) { }
        }

        public class _Octal : _Integer
        {
            public _Octal(int value) : base(TerminalType.OCTAL, value) { }
        }

        public class _Octal64 : _Integer64
        {
            public _Octal64(long value) : base(TerminalType.OCTAL64, value) { }
        }

        public class _Hex : _Integer
        {
            public _Hex(int value) : base(TerminalType.HEX, value) { }
        }

        public class _Hex64 : _Integer64
        {
            public _Hex64(long value) : base(TerminalType.HEX64, value) { }
        }

        public class _Float : _TerminalExpression<double>
        {
            public _Float(double value) : base(TerminalType.FLOAT, value) { }
        }

        public class _String : _TerminalExpression<string>
        {
            public _String(string value) : base(TerminalType.STRING, value) { }
        }

        public class _Name : _TerminalExpression<string>
        {
            public _Name(string value) : base(TerminalType.NAME, value) { }
        }


        // Non-Terminal Values //
        public class _Configuration : _Expression
        {
            public _SettingList Value { get; set; }
        }

        public class _SettingList : _Expression
        {
            //private int count;

            public List<_Setting> Value { get; set; }
            public int Count { get => Value.Count; }

            public _SettingList()
            {
                Value = new List<_Setting>();
            }

            public _SettingList(List<_Setting> value)
            {
                Value = value;
            }

            public void Add(_Setting value)
            {
                Value.Add(value);
            }
        }

        public class _Setting : _Expression
        {
            public _Name Name { get; set; }
            public _Value Value { get; set; }

            public _Setting(_Name name, _Value value)
            {
                Name = name;
                Value = value;
            }

            public _Setting()
            {

            }
        }

        public class _Value : _Expression
        {
            public _ValueOption Value { get; set; }

            public _Value(_ValueOption value)
            {
                Value = value;
            }
        }

        public class _ScalarValue : _ValueOption
        {
            public _Terminal Value { get; set; }

            public _ScalarValue(_Terminal value)
            {
                Value = value;
            }
        }

        public class _Array : _ValueOption
        {
            public _ScalarValueList Value { get; set; }

            public _Array(_ScalarValueList value)
            {
                Value = value;
            }
        }

        public class _List : _ValueOption
        {
            public _ValueList Value { get; set; }

            public _List(_ValueList value)
            {
                Value = value;
            }
        }

        public class _Group : _ValueOption
        {
            public _SettingList Value { get; set; }

            public _Group(_SettingList value)
            {
                Value = value;
            }
        }

        public class _ScalarValueList : _Expression
        {
            List<_ScalarValue> Value { get; set; }

            public _ScalarValueList()
            {
                Value = new List<_ScalarValue>();
            }

            public _ScalarValueList(List<_ScalarValue> value)
            {
                Value = value;
            }

            public void Add(_ScalarValue value)
            {
                Value.Add(value);
            }

        }

        public class _ValueList : _Expression
        {
            public List<_Value> Value { get; set; }

            public _ValueList()
            {
                Value = new List<_Value>();
            }

            public _ValueList(List<_Value> value)
            {
                Value = value;
            }

            public void Add(_Value value)
            {
                Value.Add(value);
            }
        }

        // Auxillary/Helper Classes //
        public class _Token
        {
            public string Value { get; set; }
            public TerminalType Type { get; set; }
            public string PrecedingWhiteSpace { get; set; }

            public _Token(TerminalType type, string value)
            {
                Type = type;
                Value = value;
                PrecedingWhiteSpace = "";
            }

            public _Token(TerminalType type, string value, string precedingWhiteSpace) : this(type, value)
            {
                PrecedingWhiteSpace = precedingWhiteSpace;
            }

            public override string ToString()
            {
                return PrecedingWhiteSpace + " <<" + Type.ToString().ToUpper() + " " + Value + ">>";
            }

            public string toFile()
            {
                switch(Type)
                {
                    case TerminalType.STRING:
                        return PrecedingWhiteSpace + "\"" + Value + "\"";
                    case TerminalType.SEMICOLON:
                        return PrecedingWhiteSpace + Value;
                    default:
                        return PrecedingWhiteSpace + Value;
                }
            }
        }

        public class _RegexMapItem
        {
            public TerminalType TypeValue { get; set; }
            public Regex RegexValue { get; set; }
        }

        public class _ParseResult
        {
            public int Index { get; set; }
            public _Expression Value { get; set; }

            public _ParseResult(int index, _Expression value)
            {
                Index = index;
                Value = value;
            }
        }

    }

    // Contains all of the Enumerations for the Configuration Manager //
    public partial class ConfigGramer
    {
        public enum TerminalType
        {
            BOOLEAN,      // ([Tt][Rr][Uu][Ee])|([Ff][Aa][Ll][Ss][Ee])
            STRING,       // \"([^\"\\]|\\.)*\"
            NAME,         // [A-Za-z\*][-A-Za-z0-9_\*]*
            INTEGER,      // [-+]?[0-9]+
            INTEGER64,    // [-+]?[0-9]+L(L)?
            HEX,          // 0[Xx][0-9A-Fa-f]+
            HEX64,        // 0[Xx][0-9A-Fa-f]+L(L)?
            FLOAT,        // ([-+]?([0-9]*)?\.[0-9]*([eE][-+]?[0-9]+)?)|([-+]([0-9]+)(\.[0-9]*)?[eE][-+]?[0-9]+)

            OCTAL,        // Regex not provided by libconfig: [-+]?0[0-9]*[1-9][0-9]*
            OCTAL64,      // Regex not provided by libconfig: [-+]?0[0-9]*[1-9][0-9]*L(L)?

            COLON,        // :
            EQUALS,       // =
            SEMICOLON,    // ;
            COMMA,        // ,
            OPEN_SQUARE,  // [
            CLOSE_SQUARE, // ]
            OPEN_PAREN,   // (
            CLOSE_PAREN,  // )
            OPEN_CURLY,   // {
            CLOSE_CURLY   // }
        }

        public enum SettingType
        {
            SCALAR,
            ARRAY,
            LIST,
            GROUP
        }

    }

    // Contains Data Validation code for the Configuration Manager //
    public partial class ConfigGramer
    {
        // Asserts a characteristic about a terminal's type //
        private void ParseAssert(_Token token, params TerminalType[] terminalTypes)
        {
            for (int i = 0; i < terminalTypes.Length; i++)
            {
                if (token.Type == terminalTypes[i])
                {
                    return;
                }
            }
            throw new ConfigurationException("Parse Error");
        }

        // Checks if the provided token is one of the provided terminal types //
        private bool IsTerminalType(_Token token, params TerminalType[] terminalTypes)
        {
            for (int i = 0; i < terminalTypes.Length; i++)
            {
                if (token.Type == terminalTypes[i])
                {
                    return true;
                }
            }
            return false;
        }

    }

    // Contains all of the tokenizing code for the Configuration Manager //
    public partial class ConfigGramer
    {
        /* Tokenizes Configuration */
        private List<_Token> _Tokenize(string contents)
        {
            List<_Token> tokens = new List<_Token>();

            // Order matters //
            List<_RegexMapItem> regexMapping = new List<_RegexMapItem>
            {
                new _RegexMapItem { TypeValue = TerminalType.BOOLEAN, RegexValue = booleanRegex },
                new _RegexMapItem { TypeValue = TerminalType.FLOAT, RegexValue = floatRegex },
                new _RegexMapItem { TypeValue = TerminalType.HEX64, RegexValue = hex64Regex },
                new _RegexMapItem { TypeValue = TerminalType.HEX, RegexValue = hexRegex },
                new _RegexMapItem { TypeValue = TerminalType.OCTAL64, RegexValue = octal64Regex },
                new _RegexMapItem { TypeValue = TerminalType.OCTAL, RegexValue = octalRegex },
                new _RegexMapItem { TypeValue = TerminalType.INTEGER64, RegexValue = integer64Regex },
                new _RegexMapItem { TypeValue = TerminalType.INTEGER, RegexValue = integerRegex },
                new _RegexMapItem { TypeValue = TerminalType.NAME, RegexValue = nameRegex }
            };


            Dictionary<char, TerminalType> specialCharacters = new Dictionary<char, TerminalType>
            {
                { ':', TerminalType.COLON },
                { '=', TerminalType.EQUALS },
                { ';', TerminalType.SEMICOLON },
                { ',', TerminalType.COMMA },
                { '[', TerminalType.OPEN_SQUARE },
                { ']', TerminalType.CLOSE_SQUARE },
                { '(', TerminalType.OPEN_PAREN },
                { ')', TerminalType.CLOSE_PAREN },
                { '{', TerminalType.OPEN_CURLY },
                { '}', TerminalType.CLOSE_CURLY }
            };

            /* Read string character by character */
            string preceedingWhiteSpace = "";
            for (int i = 0; i < contents.Length; ++i)
            {
                char c = contents[i];

                /* COMMENTS */
                if (c == '#')
                {
                    Match match = singleLineCommentRegex.Match(contents.Substring(i, contents.Length - i));
                    preceedingWhiteSpace += match.Value;
                    i += match.Value.Length - 1;
                }
                else if (c == '/')
                {
                    if (contents[i + 1] == '*')
                    {
                        Match match = multiLineCommentRegex.Match(contents.Substring(i, contents.Length - i));
                        preceedingWhiteSpace += match.Value;
                        i += match.Value.Length - 1;
                    }
                    else if (contents[i + 1] == '/')
                    {
                        Match match = singleLineCommentRegex.Match(contents.Substring(i, contents.Length - i));
                        preceedingWhiteSpace += match.Value;
                        i += match.Value.Length - 1;
                    }
                }

                /* STRINGS */
                else if (c == '\"')
                {
                    Match match = stringRegex.Match(contents.Substring(i, contents.Length - i));
                    tokens.Add(new _Token(TerminalType.STRING, match.Value.Trim('"'),preceedingWhiteSpace));
                    preceedingWhiteSpace = "";
                    i += match.Value.Length - 1;
                    //Debug.Write(tokens[tokens.Count - 1].ToString());                                             /* TODO : Remove this line */
                }

                /* WHITESPACE */
                else if (Char.IsWhiteSpace(c))
                { /* Just ignore it */ preceedingWhiteSpace += c.ToString(); }

                /* SPECIAL CHARACTERS */
                else if (specialCharacters.TryGetValue(c, out TerminalType typeHolder))
                {
                    tokens.Add(new _Token(typeHolder, c.ToString(), preceedingWhiteSpace));
                    preceedingWhiteSpace = "";
                    //Debug.Write(tokens[tokens.Count - 1].ToString());                                             /* TODO : Remove this line */
                }

                /* EVERYTHING ELSE - REQUIRES MORE REGEX */
                else
                {
                    int j = 0;
                    bool matchFound = false;

                    Match wordMatch = whiteSpaceRegex.Match(contents.Substring(i, contents.Length - i));
                    string word = wordMatch.Value;

                    for (j = 0; j < regexMapping.Count; j++)
                    {
                        Match match = regexMapping[j].RegexValue.Match(word);
                        if (match.Success && match.Value == word)
                        {
                            tokens.Add(new _Token(regexMapping[j].TypeValue, match.Value, preceedingWhiteSpace));
                            preceedingWhiteSpace = "";
                            i += match.Value.Length - 1;
                            matchFound = true;
                            break;
                        }
                    }

                    if (!matchFound)
                    {
                        // TODO : Make this error more verbose
                        //Debug.WriteLine(contents.Substring(i, contents.Length - i));
                        throw new ConfigurationException("Tokenizer: Encountered undefined symbol while parsing contents");
                    }
                    //Debug.Write(tokens[tokens.Count - 1].ToString());                                             /* TODO : Remove this line */
                }

            }

            // TODO: If (whitespace != "") create new semicolon token and add prior whitespace

            return tokens;
        }
    }

    // Contains all of the parsing code for the Configuration Manager //
    public partial class ConfigGramer
    {
        private _Configuration _Parse(List<_Token> tokens)
        {
            int index = 0;
            _Configuration configuration = new _Configuration();

            if (tokens.Count != 0)
            {
                _ParseResult result = _ParseSettingList(index, tokens);
                configuration.Value = (_SettingList)result.Value;
            }
            return configuration;
        }

        private _ParseResult _ParseSettingList(int index, List<_Token> tokens)
        {
            _SettingList list = new _SettingList();

            while (index < tokens.Count && IsTerminalType(tokens[index], TerminalType.NAME))
            {
                _ParseResult result = _ParseSetting(index, tokens);
                list.Add((_Setting)result.Value);
                index = result.Index;
            }

            return new _ParseResult(index, list);
        }

        private _ParseResult _ParseSetting(int index, List<_Token> tokens)
        {
            ParseAssert(tokens[index], TerminalType.NAME);
            _Name name = new _Name(tokens[index].Value);
            index++;

            ParseAssert(tokens[index], TerminalType.COLON, TerminalType.EQUALS);
            index++;

            _ParseResult valueResult = _ParseValue(index, tokens);
            index = valueResult.Index;

            if (index < tokens.Count && IsTerminalType(tokens[index], TerminalType.SEMICOLON, TerminalType.COMMA))
            {
                index++;
            }

            return new _ParseResult(index, new _Setting(name, (_Value)valueResult.Value));
        }

        private _ParseResult _ParseValue(int index, List<_Token> tokens)
        {
            _ParseResult result;
            if (IsTerminalType(tokens[index], TerminalType.OPEN_SQUARE))
            {
                result = _ParseArray(index, tokens);
            }
            else if (IsTerminalType(tokens[index], TerminalType.OPEN_PAREN))
            {
                result = _ParseList(index, tokens);
            }
            else if (IsTerminalType(tokens[index], TerminalType.OPEN_CURLY))
            {
                result = _ParseGroup(index, tokens);
            }
            else
            {
                result = _ParseScalarValue(index, tokens);
            }

            index = result.Index;

            return new _ParseResult(index, new _Value((_ValueOption)result.Value));
        }

        private _ParseResult _ParseValueList(int index, List<_Token> tokens)
        {
            _ValueList list = new _ValueList();

            while (!IsTerminalType(tokens[index], TerminalType.CLOSE_PAREN))
            {
                // TODO: Assumes that there exists at least one value in the list
                _ParseResult result = _ParseValue(index, tokens);
                list.Add((_Value)result.Value);
                index = result.Index;

                if (IsTerminalType(tokens[index], TerminalType.COMMA))
                {
                    index++;
                }
                else if (!IsTerminalType(tokens[index], TerminalType.CLOSE_PAREN))
                {
                    throw new ConfigurationException("Expected end of value-list at token " + index.ToString() + " \"" + tokens[index].Value + "\"");
                }
            }

            return new _ParseResult(index, list);
        }

        private _ParseResult _ParseScalarValue(int index, List<_Token> tokens)
        {
            _Terminal terminal;

            switch (tokens[index].Type)
            {
                case TerminalType.BOOLEAN:
                    terminal = new _Boolean(Convert.ToBoolean(tokens[index].Value));
                    break;

                case TerminalType.INTEGER:
                    try
                    {
                        terminal = new _Integer(Convert.ToInt32(tokens[index].Value));
                    }
                    catch (OverflowException)
                    {
                        terminal = new _Integer64(Convert.ToInt64(tokens[index].Value));
                        tokens[index].Type = TerminalType.INTEGER64;
                    }
                    break;

                case TerminalType.INTEGER64:
                    string integerString = tokens[index].Value.TrimEnd('L');
                    terminal = new _Integer64(Convert.ToInt64(integerString));
                    break;

                case TerminalType.OCTAL:
                    try
                    {
                        terminal = new _Octal(Convert.ToInt32(tokens[index].Value, 8));
                    }
                    catch (OverflowException)
                    {
                        terminal = new _Octal64(Convert.ToInt64(tokens[index].Value, 8));
                        tokens[index].Type = TerminalType.OCTAL64;
                    }
                    break;

                case TerminalType.OCTAL64:
                    string octalString = tokens[index].Value.TrimEnd('L');
                    terminal = new _Octal64(Convert.ToInt64(octalString, 8));
                    break;

                case TerminalType.HEX:
                    try
                    {
                        terminal = new _Hex(Convert.ToInt32(tokens[index].Value, 16));
                    }
                    catch (OverflowException)
                    {
                        terminal = new _Hex64(Convert.ToInt64(tokens[index].Value, 16));
                        tokens[index].Type = TerminalType.HEX64;
                    }
                    break;

                case TerminalType.HEX64:
                    string hexString = tokens[index].Value.TrimEnd('L');
                    terminal = new _Hex64(Convert.ToInt64(hexString, 16));
                    break;

                case TerminalType.FLOAT:
                    terminal = new _Float(Convert.ToDouble(tokens[index].Value));
                    break;

                case TerminalType.STRING:
                    // Enable string chaining
                    // TODO: This will force all strings to become single-lined
                    string terminalValue = tokens[index].Value;
                    while ((index + 1 < tokens.Count) && IsTerminalType(tokens[index + 1], TerminalType.STRING))
                    {
                        index++;
                        terminalValue += tokens[index].Value;
                    }

                    terminal = new _String(terminalValue);
                    break;

                default:
                    throw new ConfigurationException("Invalid scalar encountered while parsing: " + tokens[index].Value);
            }

            index++;

            return new _ParseResult(index, new _ScalarValue(terminal));
        }

        private _ParseResult _ParseScalarValueList(int index, List<_Token> tokens)
        {
            _ScalarValueList list = new _ScalarValueList();

            while (!IsTerminalType(tokens[index], TerminalType.CLOSE_SQUARE))
            {
                // TODO: Assumes that there is at least one value in the list
                _ParseResult result = _ParseScalarValue(index, tokens);
                list.Add((_ScalarValue)result.Value);
                index = result.Index;

                if (IsTerminalType(tokens[index], TerminalType.COMMA))
                {
                    index++;
                }
                else if (!IsTerminalType(tokens[index], TerminalType.CLOSE_SQUARE))
                {
                    throw new ConfigurationException("Expected end of scalar-value-list at \"" + tokens[index].Value + "\"");
                }
            }

            return new _ParseResult(index, list);
        }

        private _ParseResult _ParseArray(int index, List<_Token> tokens)
        {
            ParseAssert(tokens[index], TerminalType.OPEN_SQUARE);
            index++;

            if (IsTerminalType(tokens[index], TerminalType.CLOSE_SQUARE))
            {
                index++;
                return new _ParseResult(index, new _Array(new _ScalarValueList()));
            }

            _ParseResult result = _ParseScalarValueList(index, tokens);
            index = result.Index;

            ParseAssert(tokens[index], TerminalType.CLOSE_SQUARE);
            index++;

            return new _ParseResult(index, new _Array((_ScalarValueList)result.Value));
        }

        private _ParseResult _ParseList(int index, List<_Token> tokens)
        {
            ParseAssert(tokens[index], TerminalType.OPEN_PAREN);
            index++;

            if (IsTerminalType(tokens[index], TerminalType.CLOSE_PAREN))
            {
                index++;
                return new _ParseResult(index, new _List(new _ValueList()));
            }

            _ParseResult result = _ParseValueList(index, tokens);
            index = result.Index;

            ParseAssert(tokens[index], TerminalType.CLOSE_PAREN);
            index++;

            return new _ParseResult(index, new _List((_ValueList)result.Value));
        }

        private _ParseResult _ParseGroup(int index, List<_Token> tokens)
        {
            ParseAssert(tokens[index], TerminalType.OPEN_CURLY);
            index++;

            if (IsTerminalType(tokens[index], TerminalType.CLOSE_CURLY))
            {
                index++;
                return new _ParseResult(index, new _Group(new _SettingList()));
            }

            _ParseResult result = _ParseSettingList(index, tokens);
            index = result.Index;

            ParseAssert(tokens[index], TerminalType.CLOSE_CURLY);
            index++;

            return new _ParseResult(index, new _Group((_SettingList)result.Value));
        }
    }

    // Contains all of the the deliverable code for the Configuration Manager //
    public partial class ConfigGramer
    {
        public class ConfigurationException : Exception
        {
            public ConfigurationException(string message) : base(message) { }
            public ConfigurationException(string message, Exception exception) : base(message, exception) { }
        }

        public class ItemValue<T>
        {
            public T Value { get; }
            public bool IsValid { get; }
            public ItemValue(T value, bool valid = true)
            {
                Value = value;
                IsValid = valid;
            }

        }


        public class Item
        {
            public ItemType context;
            public _Expression value;

            public Item()
            {
                context = ItemType.EMPTY;
                value = null;
            }

            public Item(ItemType context, _Expression value)
            {
                this.context = context;
                this.value = value;
            }

            public Item(Item item)
            {
                context = item.context;
                value = item.value;
            }

            public bool IsEmpty()
            {
                return context == ItemType.EMPTY;
            }

            public string GetString()
            {
                if (context == ItemType.STRING)
                {
                    return ((_String)value).Value;
                }
                return "";
            }

            public bool GetBoolean()
            {
                if (context == ItemType.BOOLEAN)
                {
                    return ((_Boolean)value).Value;
                }
                return false;
            }
            public double GetFloat()
            {
                if(context == ItemType.FLOAT)
                {
                    return ((_Float)value).Value;
                }
                return 0;
            }

            internal List<string> GetStringList()
            {
                List<string> list = new List<String>();
                if(context == ItemType.VALUE_LIST)
                {
                    _ValueList SettingList = (_ValueList)value;
                    foreach(_Value v in SettingList.Value)
                    {
                        _ScalarValue s = (_ScalarValue)v.Value;
                        _String str = (_String)s.Value;
                        list.Add(str.Value);
                    }

                }
                else
                {
                    throw new NotImplementedException();
                }
                return list;
            }
        }

        public Item GetItem(params ASTQualifier[] qualifiers)
        {
            Item item = new Item();
            return GetItem(item, qualifiers);
        }

        public Item GetItem(Item item1, params ASTQualifier[] qualifiers)
        {
            Item item = new Item(item1);

            // The verbosity here is strictly for readability //
            for (int qi = 0; qi < qualifiers.Length; qi++)
            {
                switch (qualifiers[qi].itemType)
                {
                    case ItemType.CONFIGURATION:
                        item.context = ItemType.CONFIGURATION;
                        item.value = _BaseConfiguration;
                        break;

                    case ItemType.SETTING_LIST:
                        if (item.context == ItemType.CONFIGURATION)
                        {
                            _Configuration configuration = (_Configuration)item.value;
                            item.context = ItemType.SETTING_LIST;
                            item.value = configuration.Value;
                        }
                        else if (item.context == ItemType.GROUP)
                        {
                            _Group group = (_Group)item.value;
                            item.context = ItemType.SETTING_LIST;
                            item.value = group.Value;
                        }
                        break;

                    case ItemType.SETTING:
                        _SettingList settingList = (_SettingList)item.value;
                        for (int i = 0; i < settingList.Value.Count; i++)
                        {
                            if (settingList.Value[i].Name.Value == qualifiers[qi].itemQualifier)
                            {
                                item.context = ItemType.SETTING;
                                item.value = settingList.Value[i];
                                break;
                            }
                        }
                        break;

                    case ItemType.NAME:
                        if (item.context == ItemType.SETTING)
                        {
                            _Setting setting = (_Setting)item.value;
                            item.context = ItemType.NAME;
                            item.value = setting.Name;
                        }
                        break;

                    case ItemType.VALUE:
                        if (item.context == ItemType.SETTING)
                        {
                            _Setting setting = (_Setting)item.value;
                            item.context = ItemType.VALUE;
                            item.value = setting.Value;
                        }
                        if (item.context == ItemType.VALUE_LIST && qualifiers[qi].itemQualifier != null)
                        {
                            int index = Int32.Parse(qualifiers[qi].itemQualifier);

                            _ValueList valueList = (_ValueList)item.value;

                            item.context = ItemType.VALUE;
                            item.value = valueList.Value[index];
                        }
                        break;

                    case ItemType.SCALAR_VALUE:
                        if (item.context == ItemType.VALUE)
                        {
                            _Value value = (_Value)item.value;

                            // TODO: Error check that value is a scalar value
                            item.context = ItemType.SCALAR_VALUE;
                            item.value = value.Value;
                        }
                        break;


                    case ItemType.ARRAY:
                        if (item.context == ItemType.VALUE)
                        {
                            _Value value = (_Value)item.value;
                            item.context = ItemType.SCALAR_VALUE;
                            item.value = value.Value;
                        }
                        break;

                    case ItemType.LIST:
                        if (item.context == ItemType.VALUE)
                        {
                            _Value value = (_Value)item.value;
                            item.context = ItemType.LIST;
                            item.value = value.Value;
                        }
                        break;

                    case ItemType.GROUP:
                        if (item.context == ItemType.VALUE)
                        {
                            _Value value = (_Value)item.value;
                            item.context = ItemType.GROUP;
                            item.value = value.Value;
                        }
                        break;

                    case ItemType.VALUE_LIST:
                        if (item.context == ItemType.LIST)
                        {
                            _List list = (_List)item.value;
                            item.context = ItemType.VALUE_LIST;
                            item.value = list.Value;
                        }
                        break;

                    case ItemType.STRING:
                        if (item.context == ItemType.SCALAR_VALUE)
                        {
                            _ScalarValue scalarValue = (_ScalarValue)item.value;
                            item.context = ItemType.STRING;
                            item.value = scalarValue.Value;
                        }
                        break;

                    case ItemType.BOOLEAN:
                        if (item.context == ItemType.SCALAR_VALUE)
                        {
                            _ScalarValue scalarValue = (_ScalarValue)item.value;
                            item.context = ItemType.BOOLEAN;
                            item.value = scalarValue.Value;
                        }
                        break;
                    case ItemType.FLOAT:
                        if (item.context == ItemType.SCALAR_VALUE)
                        {
                            _ScalarValue scalarValue = (_ScalarValue)item.value;
                            item.context = ItemType.FLOAT;
                            item.value = scalarValue.Value;
                        }
                        break;

                    default:
                        item.context = ItemType.EMPTY;
                        item.value = null;
                        break;
                }
            }

            return item;
        }

        public void Load(string contents)
        {
            try
            {
                // string contents = File.ReadAllText(path);
                //_BaseConfiguration = new _Configuration();
                _Tokens = _Tokenize(contents);
                _BaseConfiguration = _Parse(_Tokens);

            }
            catch (Exception)
            {
                Debug.WriteLine("Error Parsing File");
            }
        }

        // Looks for a certain setting and return null if it is not found
        /*public Item GetSetting(string name)
        {
            Item item = new Item(this);
            return item.GetSetting(name);
        }*/

        // checks the next token to see if its a semicolon and inserts one if its missing
        private int AddMissingSemicolon(int tokenIndex)
        {
            if (_Tokens[tokenIndex + 1].Type != TerminalType.SEMICOLON)
            { // insert a missing semicolon
                _Tokens.Insert(tokenIndex + 1, new _Token(TerminalType.SEMICOLON, ";"));
            }
            return tokenIndex;
        }

        private int ModifyAddons(int tokenIndex, List<AddonItem> addonItems)
        // TODO: handle adding addonItems that aren't included in the curent config file
        {
            int addonIndex = 0;
            bool enabledSet;
            while (addonIndex < addonItems.Count())
            {
                enabledSet = false;
                while (_Tokens[tokenIndex].Type != TerminalType.CLOSE_CURLY)
                {
                    if (addonItems[addonIndex].Updated && _Tokens[tokenIndex].Type == TerminalType.NAME)
                    {
                        String newVal = "";
                        switch (_Tokens[tokenIndex].Value)
                        {
                            case "name":
                                newVal = addonItems[addonIndex].Name;
                                break;
                            case "description":
                                newVal = addonItems[addonIndex].Description;
                                break;
                            case "version":
                                newVal = addonItems[addonIndex].PrettyVersion;
                                break;
                            case "type":
                                newVal = addonItems[addonIndex].Type;
                                break;
                            case "path":
                                newVal = addonItems[addonIndex].Path;
                                break;
                            case "enabled":
                                enabledSet = true;
                                newVal = addonItems[addonIndex].Enabled ? "true" : "false";
                                break;
                            default:
                                goto ModifyAddons_skip;  // If we don't recognize the tolken name don't skip 2
                        }
                        tokenIndex += 2; // <name> AND <=> OR <:>
                        _Tokens[tokenIndex].Value = newVal;

                        // TODO This is a work around. Find out whiy the whitespace isn't parced and saved corectly
                        if (_Tokens[tokenIndex].PrecedingWhiteSpace == "") {
                            _Tokens[tokenIndex].PrecedingWhiteSpace = " ";
                        }
                        // set the value of the next tolken to the new modifyed value
                        tokenIndex = AddMissingSemicolon(tokenIndex);
                    }
ModifyAddons_skip:
                    tokenIndex++;
                }
                // To always add the enabled field in  the config file use
                if (!enabledSet && addonItems[addonIndex].Updated)
                // To only add an enabled field if changing from default case use (DEFAULT is not defined)
                // if (!enabledSet && addonItems[addonIndex].Enabled != DEFAULT)
                {
                    _Tokens.Insert(tokenIndex++, new _Token(TerminalType.NAME, "enabled", "\r\n\t\t"));
                    _Tokens.Insert(tokenIndex++, new _Token(TerminalType.EQUALS, " = "));
                    _Tokens.Insert(tokenIndex++, new _Token(TerminalType.BOOLEAN, addonItems[addonIndex].Enabled ? "true" : "false"));
                    _Tokens.Insert(tokenIndex++, new _Token(TerminalType.COLON, ";"));
                }
                tokenIndex++; // <}>
                addonItems[addonIndex++].Updated = false;
            }
            tokenIndex = AddMissingSemicolon(tokenIndex);

            return tokenIndex;
        }

        private int ModifyPlugins(int tokenIndex, List<PluginItem> pluginItems)
        {
            int pluginIndex = 0;
            bool enabledSet;
            while (pluginIndex < pluginItems.Count())
            {
                enabledSet = false;
                while (_Tokens[tokenIndex].Type != TerminalType.CLOSE_CURLY)
                {
                    if (pluginItems[pluginIndex].Updated && _Tokens[tokenIndex].Type == TerminalType.NAME)
                    {
                        String newVal = "";
                        switch (_Tokens[tokenIndex].Value)
                        {
                            case "name":
                                newVal = pluginItems[pluginIndex].Name;
                                break;
                            case "description":
                                newVal = pluginItems[pluginIndex].Description;
                                break;
                            case "version":
                                newVal = pluginItems[pluginIndex].PrettyVersion;
                                break;
                            case "type":
                                newVal = pluginItems[pluginIndex].Type;
                                break;
                            case "handler":
                                newVal = pluginItems[pluginIndex].Handler;
                                break;
                            case "path":
                                newVal = pluginItems[pluginIndex].Path;
                                break;
                            case "enabled":
                                enabledSet = true;
                                newVal = pluginItems[pluginIndex].Enabled ? "true" : "false";
                                break;
                            default:
                                goto ModifyPlugins_skip;  // If we don't recognize the tolken name don't skip 2
                        }
                        tokenIndex += 2; // <name> AND <=> OR <:>
                        _Tokens[tokenIndex].Value = newVal;

                        tokenIndex = AddMissingSemicolon(tokenIndex);
                    }
ModifyPlugins_skip:
                    tokenIndex++;
                }
                // Add the enabled field in the config file if not in modified plugin
                if (!enabledSet && pluginItems[pluginIndex].Updated)
                {
                    _Tokens.Insert(tokenIndex++, new _Token(TerminalType.NAME, "enabled", "\r\n\t\t"));
                    _Tokens.Insert(tokenIndex++, new _Token(TerminalType.EQUALS, " = "));
                    _Tokens.Insert(tokenIndex++, new _Token(TerminalType.BOOLEAN, pluginItems[pluginIndex].Enabled ? "true" : "false"));
                    _Tokens.Insert(tokenIndex++, new _Token(TerminalType.COLON, ";"));
                }
                tokenIndex++; // <}>
                pluginItems[pluginIndex++].Updated = false;
            }
            /*if (pluginIndex < pluginItems.Count)
            {
                // TODO: handle adding pluginItems that where added after loading but before saving the config file
            }*/
            tokenIndex = AddMissingSemicolon(tokenIndex);

            return tokenIndex;
        }



        // subroutine of outputModifyAggregation
        private int ModifySufficient(int tokenIndex, Aggregation aggreItem)
        {
            while (_Tokens[tokenIndex].Type != TerminalType.CLOSE_CURLY)
            {
                if (_Tokens[tokenIndex].Type == TerminalType.NAME)
                {
                    List<String> members;
                    switch (_Tokens[tokenIndex].Value)
                    {
                        case "congress_group":
                            members = aggreItem.Congress;
                            break;
                        case "necessary_group":
                            members = aggreItem.NecessaryGroup;
                            break;
                        default:
                            goto ModifySufficient_append;
                    }
                
                    // TODO actualy change the tolken list
                    tokenIndex += 3; // <name> AND <=> OR <:> AND <)>
                    while (_Tokens[tokenIndex].Type != TerminalType.CLOSE_PAREN)
                    {
                        _Tokens.RemoveAt(tokenIndex);
                    }
                    for (int i = 0; i < members.Count; i++)
                    {
                        if (i != 0)
                        {
                            _Tokens.Insert(tokenIndex++, new _Token(TerminalType.COMMA,", "));
                        }
                        _Tokens.Insert(tokenIndex++,new _Token(TerminalType.STRING, members.ElementAt(i)));
                    }
                }
                ModifySufficient_append:
                tokenIndex++; // append any tokens I don't recognize as names
            }
            return tokenIndex;
        }
        private int ModifyAggregation(int tokenIndex, Aggregation aggreItem)
        {
                while (_Tokens[tokenIndex].Type != TerminalType.CLOSE_CURLY)
                {
                    if (aggreItem.Updated && _Tokens[tokenIndex].Type == TerminalType.NAME)
                    {
                        switch (_Tokens[tokenIndex].Value)
                        {
                            case "congress_threshold":
                                tokenIndex++; // name
                                tokenIndex++; // = OR :
                                _Tokens[tokenIndex].Value = (Double.Parse(aggreItem.Threshold)/100).ToString();
                            break;
                            case "sufficient":
                                tokenIndex = ModifySufficient(tokenIndex, aggreItem);
                                break;
                            default:
                                break;
                        }
                    }
                    tokenIndex++;
                }
                // To change this to only add an enabled field if changing from default case use
                // if (!enabledSet && pluginItems[pluginIndex].Enabled != DEFAULT)
                tokenIndex++;
                aggreItem.Updated = false;
            return tokenIndex;
        }

        public String MakeConfiguration(List<AddonItem> addonItems, List<PluginItem> pluginItems, Aggregation aggreItem)
        {
            StringBuilder fileText = new StringBuilder();

            aggreItem.setGroups(pluginItems);

            for(int i=0; i < _Tokens.Count(); i++)
            {
                switch (_Tokens[i].Type)
                {
                    case TerminalType.NAME:
                        String name = _Tokens[i].Value.ToLower();
                        switch (name)
                        {
                            case "addons":
                                i = ModifyAddons(i, addonItems);
                                break;
                            case "plugins":
                                i = ModifyPlugins(i, pluginItems);
                                break;
                            case "aggregation": // handle updating aggregation
                                i = ModifyAggregation(i, aggreItem);
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
            }
            for(int i =0; i < _Tokens.Count(); i++)
            {
                fileText.Append(_Tokens[i].toFile());
            }
            return fileText.ToString();
        }
    }
}
