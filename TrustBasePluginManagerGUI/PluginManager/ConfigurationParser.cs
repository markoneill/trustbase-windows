using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace PluginManager
{

    /**
     * Simple Parser following libconfig rules:
     * 
     
        RULES:
        configuration = setting-list | empty
        setting-list = setting | setting-list setting
        setting = name(":" | "=") value(";" | "," | empty)
        value = scalar-value | array | list | group
        value-list = value | value-list "," value
        scalar-value = boolean | integer | integer64 | hex | hex64 | float | string
        scalar-value-list = scalar-value | scalar-value-list "," scalar-value
        array = "["(scalar - value - list | empty) "]"
        list = "(" (value-list | empty) ")"
        group = "{" (setting-list | empty) "}"
        empty =

        TERMINALS:
        Terminals are defined below as regular expressions:
        
        boolean	([Tt][Rr][Uu][Ee])|([Ff][Aa][Ll][Ss][Ee])
        string	\"([^\"\\]|\\.)*\"
        name	[A-Za-z\*][-A-Za-z0-9_\*]*
        integer	[-+]?[0-9]+
        integer64	[-+]?[0-9]+L(L)?
        hex	0[Xx][0-9A-Fa-f]+
        hex64	0[Xx][0-9A-Fa-f]+L(L)?
        float	([-+]?([0-9]*)?\.[0-9]*([eE][-+]?[0-9]+)?)|([-+]([0-9]+)(\.[0-9]*)?[eE][-+]?[0-9]+)

    */

    public class ConfigurationParser
    {

        public class ConfigurationException : Exception
        {
            public Exception original;
            public ConfigurationException(string message) : base(message) { }
            public ConfigurationException(string message, Exception e) : base(message, e) { }
        }

        private class _Configuration
        {
            public _SettingList Settings { get; set; }
            public _Configuration() { this.Settings = new _SettingList(); }
           
        }

        private class _SettingList
        {
            public _Setting setting { get; set; }
            public _SettingList settingList { get; set; }

            public _SettingList()
            {
                this.setting = null;
                this.settingList = null;
            }

            public _SettingList(_Setting setting, _SettingList settingList)
            {
                this.setting = setting;
                this.settingList = settingList;
            }
        }

        private class _Setting
        {
            public string name { get; set; }
            public string value { get; set; }
        }

        private class _Value
        {

        }

        private class _ValueList
        {

        }
    
        private class _ScalarValue
        {

        }

        private class _ScalarValueList
        {

        }

        private class _Array
        {

        }

        private class _List
        {

        }

        private class _Group
        {

        }

        private class TerminalRegexTuple
        {
            public TerminalType type { get; set; }
            public Regex regex { get; set; }

            public TerminalRegexTuple(TerminalType type, Regex regex)
            {
                this.type = type;
                this.regex = regex;
            }
        }

        private class Terminal
        {
            public TerminalType type { get; set; }
            public string value { get; set; }

            public Terminal(TerminalType type, string value)
            {
                this.type = type;
                this.value = value;
            }

            public override string ToString()
            {
                return "<<" + type.ToString().ToUpper() + " " + value.ToString() + ">>";
            }
        }

        enum TerminalType
        {
            BOOLEAN,      // ([Tt][Rr][Uu][Ee])|([Ff][Aa][Ll][Ss][Ee])
            STRING,       // \"([^\"\\]|\\.)*\"
            NAME,         // [A-Za-z\*][-A-Za-z0-9_\*]*
            INTEGER,      // [-+]?[0-9]+
            INTEGER64,    // [-+]?[0-9]+L(L)?
            HEX,          // 0[Xx][0-9A-Fa-f]+
            HEX64,        // 0[Xx][0-9A-Fa-f]+L(L)?
            FLOAT,        // ([-+]?([0-9]*)?\.[0-9]*([eE][-+]?[0-9]+)?)|([-+]([0-9]+)(\.[0-9]*)?[eE][-+]?[0-9]+)

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

        /********************************************************************************/
        /********************************************************************************/
        public ConfigurationParser() { }
        public void Load(string contents)
        {
            _Configuration configurations = new _Configuration();

            if (contents.Length == 0) { return; }

            List<Terminal> tokens = this.Tokenize(contents);
            //_Configuration configuration = this.Parse(tokens);
            

        }

        /********************************************************************************/
        /********************************************************************************/
 
        /* Tokenizes Configuration */
        private List<Terminal> Tokenize(string contents)
        {
            TerminalType typeHolder;
            List<Terminal> terminals = new List<Terminal>();

            /* REGEX to identify Terminals */
            Regex stringRegex = new Regex(@"""([^""\\]|\\.)*""");
            Regex booleanRegex = new Regex(@"([Tt][Rr][Uu][Ee])|([Ff][Aa][Ll][Ss][Ee])");
            Regex nameRegex = new Regex(@"[A-Za-z\*][-A-Za-z0-9_\*]*");
            Regex integerRegex = new Regex(@"[-+]?[0-9]+");
            Regex integer64Regex = new Regex(@"[-+]?[0-9]+L(L)?");
            Regex hexRegex = new Regex(@"0[Xx][0-9A-Fa-f]+");
            Regex hex64Regex = new Regex(@"0[Xx][0-9A-Fa-f]+L(L)?");
            Regex floatRegex = new Regex(@"([-+]?([0-9]*)?\.[0-9]*([eE][-+]?[0-9]+)?)|([-+]([0-9]+)(\.[0-9]*)?[eE][-+]?[0-9]+)");

            /* Order matters */
            List<TerminalRegexTuple> regexMapping = new List<TerminalRegexTuple>
            {
                new TerminalRegexTuple(TerminalType.BOOLEAN, booleanRegex),
                new TerminalRegexTuple(TerminalType.NAME, nameRegex),
                new TerminalRegexTuple(TerminalType.FLOAT, floatRegex),
                new TerminalRegexTuple(TerminalType.HEX64, hex64Regex),
                new TerminalRegexTuple(TerminalType.HEX, hexRegex),
                new TerminalRegexTuple(TerminalType.INTEGER64, integer64Regex),
                new TerminalRegexTuple(TerminalType.INTEGER, integerRegex)
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
            int i = 0;
            for (i = 0; i < contents.Length; ++i)
            {
                char c = contents[i];

                /* COMMENTS */
                if (c == '#')
                {
                    while (contents[++i] != '\n') { }
                }
                else if (c == '/')
                {
                    if (contents[++i] == '*')
                    {
                        while (!(contents[++i] == '/' && contents[i - 1] == '*')) { }
                    }
                    else if (contents[i] == '/')
                    {
                        while (contents[++i] != '\n') { }
                    }
                }

                /* STRINGS */
                else if (c == '\"')
                {
                    Match match = stringRegex.Match(contents.Substring(i, contents.Length - i));
                    terminals.Add(new Terminal(TerminalType.STRING, match.Value.Trim('"')));
                    i += match.Value.Length - 1;
                    //Debug.Write(terminals[terminals.Count - 1].ToString());                                             /* TODO : Remove this line */
                }

                /* WHITESPACE */
                else if (Char.IsWhiteSpace(c))
                { /* Just ignore it */ Debug.Write(c); }

                /* SPECIAL CHARACTERS */
                else if (specialCharacters.TryGetValue(c, out typeHolder))
                {
                    terminals.Add(new Terminal(typeHolder, c.ToString()));
                    //Debug.Write(terminals[terminals.Count - 1].ToString());                                             /* TODO : Remove this line */
                }

                /* EVERYTHING ELSE - REQUIRES MORE REGEX */
                else
                {
                    int j = 0;
                    bool matchFound = false;
                    for (j = 0; j < regexMapping.Count; j++ )
                    {
                        Match match = regexMapping[j].regex.Match(contents.Substring(i, contents.Length - i));
                        if (match.Success)
                        {
                            terminals.Add(new Terminal(regexMapping[j].type, match.Value));
                            i += match.Value.Length - 1;
                            matchFound = true;
                            break;
                        }                        
                    }

                    if (!matchFound)
                    {
                        //Debug.WriteLine(contents.Substring(i, contents.Length - i));
                        throw new ConfigurationException("Tokenizer: Encountered undefined symbol while parsing contents");
                    }
                    //Debug.Write(terminals[terminals.Count - 1].ToString());                                             /* TODO : Remove this line */
                }

            }

            return terminals;
        }

        /********************************************************************************/
        /********************************************************************************/


/*        private _Configuration Parse(List<Terminal> tokens)
        {
            _Configuration configuration = new _Configuration();
            if (tokens.Count > 0)
            {
                configuration.Settings = ParseSettingList(tokens);
            }
            return configuration;
        }


        private _SettingList ParseSettingList(List<Terminal> tokens)
        {
            _SettingList list = ParseSetting(tokens);
            if (list.settingList != null)
            {
               // ParseSettingList()
            }
            return list;
        }

        private _SettingList ParseSettingList(List<Terminal> tokens, _SettingList list)
        {
            return null;
        }

        private _SettingList ParseSetting(List<Terminal> tokens)
        {
            /* setting = name (":" | "=") value (";" | "," | empty) */
/*            _Setting newSetting = new _Setting();

            int i = 0;

            ParseAssert(tokens[0], TerminalType.NAME);
            ParseAssert(tokens[1], TerminalType.EQUALS, TerminalType.COLON);

            /* Check if it is a simple scalar */
/*            if (IsTerminalType(tokens[2], 
                TerminalType.BOOLEAN, 
                TerminalType.INTEGER, 
                TerminalType.INTEGER64, 
                TerminalType.HEX,
                TerminalType.HEX64,
                TerminalType.FLOAT,
                TerminalType.STRING))
            {
                i = 3;
                newSetting.name = tokens[0].value;
                newSetting.value = tokens[2].value;

                if (tokens.Count > 3)
                {
                    if (IsTerminalType(tokens[4], TerminalType.SEMICOLON, TerminalType.COMMA))
                    {
                        i++;
                    }
                }
                
                return new _SettingList(newSetting, ParseSetting(tokens.GetRange(i, tokens.Count - i)));
            }

            return null;
        }

        private _Value ParseValue(List<Terminal> tokens)
        {
            //TODO: fill out this function
            return null;
        }

        private _ValueList ParseValueList(List<Terminal> tokens)
        {
            //TODO: fill out this function
            return null;
        }

        private _ScalarValue ParseScalarValue(List<Terminal> tokens)
        {
            //TODO: fill out this function
            return null;
        }

        private _ScalarValueList ParseScalarValueList(List<Terminal> tokens)
        {
            //TODO: fill out this function
            return null;
        }

        private _Array ParseArray(List<Terminal> tokens)
        {
            //TODO: fill out this function
            return null;
        }

        private _List ParseList(List<Terminal> tokens)
        {
            //TODO: fill out this function
            return null;
        }

        private _Group ParseGroup(List<Terminal> tokens)
        {
            //TODO: fill out this function
            return null;
        }

        /* DATA VALIDATION */

        /**
         * Checks if the provided token is one of the terminal types given.
         * 
         * @throws ConfigurationException if the token does not match any of the terminal types provided
         */
/*        private void ParseAssert(Terminal token, params TerminalType[] terminalTypes)
        {
            for (int i = 0; i < terminalTypes.Length; i++)
            {
                if (token.type == terminalTypes[i])
                {
                    return;
                }
            }
            throw new ConfigurationException("Parse Error");
        }

        /**
         * Checks if the provided token is one of the provided terminal types
         */
/*        private bool IsTerminalType(Terminal token, params TerminalType[] terminalTypes)
        {
            for (int i = 0; i < terminalTypes.Length; i++)
            {
                if (token.type == terminalTypes[i])
                {
                    return true;
                }
            }
            return false;
        }
*/


    }
}
