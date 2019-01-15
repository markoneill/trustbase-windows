using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PluginManager
{
    public class ASTQualifier
    {
        public ItemType itemType;
        public string itemQualifier;
        public string configIdentifier; // name in config file

        public ASTQualifier(ItemType itemType)
        {
            this.itemType = itemType;
            itemQualifier = null;
        }

        public ASTQualifier(ItemType itemType, string itemQualifier)
        {
            this.itemType = itemType;
            this.itemQualifier = itemQualifier;
        }
    }
}
