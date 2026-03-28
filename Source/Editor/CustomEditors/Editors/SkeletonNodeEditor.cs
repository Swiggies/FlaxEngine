// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;
using System.Linq;
using System;
using System.Text;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom editor for picking skeleton node. Instead of choosing node index or entering node text it shows a combo box with simple tag picking by name.
    /// </summary>
    public sealed class SkeletonNodeEditor : CustomEditor
    {
        private ComboBoxElement element;
        private ButtonElement elementButton;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var contextMenu = new ItemsListContextMenu();

            elementButton = layout.Button("Pick a bone");
            elementButton.Button.ButtonClicked += (b) => contextMenu.Show(b.Parent, b.BottomLeft);
            // Set node names
            if (ParentEditor != null
                && ParentEditor.Values.Count == 1 && (ParentEditor.Values[0] is Actor animatedActor)
                && animatedActor.Parent is AnimatedModel animatedModel && animatedModel.SkinnedModel
                && !animatedModel.SkinnedModel.WaitForLoaded())
            {
                var nodes = animatedModel.SkinnedModel.Nodes;
                for (int nodeIndex = 0; nodeIndex < nodes.Length; nodeIndex++)
                {
                    if (nodes[nodeIndex].ParentIndex == -1)
                    {
                        var sb = new StringBuilder();
                        BuildNodeTree(contextMenu, nodes, nodeIndex, 0);
                    }
                }
            }
        }

        private void BuildNodeTree(ItemsListContextMenu cm, SkeletonNode[] nodes, int nodeIndex, int indentLevel)
        {
            ItemsListContextMenu.Item item = new ItemsListContextMenu.Item();
            item.Name = $"{new string(' ', indentLevel)}{nodes[nodeIndex].Name}";
            item.Clicked += ItemClicked;
            item.SortScore = nodeIndex;

            cm.AddItem(item);

            for (int i = 0; i < nodes.Length; i++)
            {
                if (nodes[i].ParentIndex == nodeIndex)
                {
                    BuildNodeTree(cm, nodes, i, indentLevel + 1);
                }
            }
        }

        private void ItemClicked(ItemsListContextMenu.Item obj)
        {
            Debug.Log(obj.Name);
            string value = obj.Name;
            SetValue(value);
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            string value = comboBox.SelectedItem;
            SetValue(value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values on many actor selected
            }
            else
            {
                string value = (string)Values[0];
                elementButton.Button.Text = value;
            }
        }
    }
}
