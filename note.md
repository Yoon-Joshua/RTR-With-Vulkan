| type | Vertex | MSAA  | subpass |
|-|-|-|-|
|阴影贴图|-|-|0|
|G-Buffer|-|-||
|白炉|-|4|||
|PRT|prt|4|-|

blend attachment 的数量根据framebuffer的数量来确定

所有的model矩阵用Push constant， view和proj矩阵用uniform buffer.

