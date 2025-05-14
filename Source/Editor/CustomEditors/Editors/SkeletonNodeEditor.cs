// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;
using System.Linq;
using System;

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

            var cm = new ItemsListContextMenu();

            elementButton = layout.Button("Pick a bone");
            elementButton.Button.ButtonClicked += (b) => cm.Show(b, b.Location);
            // Set node names
            if (ParentEditor != null
                && ParentEditor.Values.Count == 1 && (ParentEditor.Values[0] is Actor boneSocket)
                && boneSocket.Parent is AnimatedModel animatedModel && animatedModel.SkinnedModel
                && !animatedModel.SkinnedModel.WaitForLoaded())
            {
                var nodes = animatedModel.SkinnedModel.Nodes;
                for (int nodeIndex = 0; nodeIndex < nodes.Length; nodeIndex++)
                {
                    ItemsListContextMenu.Item item = new ItemsListContextMenu.Item();
                    item.Name = nodes[nodeIndex].Name;
                    item.Clicked += ItemClicked;

                    cm.AddItem(item);
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
