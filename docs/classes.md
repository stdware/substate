# Classes

## Node

+ Node
+ BytesNode
+ VectorNode
+ SheetNode
+ MappingNode

## Serialization

### Node

Generally, all properties are written sequentially in order.

### Action

When serializing a node, create a table of inserted or removed nodes of the current node firstly, then write other data.

```
0x0         string      "SSTA"
0x4         int32       action_type
0x8         int32       nodes_table_size
0xC         int32       inserted_count
0x10        int32       inserted_node_id_1
0x14        int32       inserted_node_id_2
...
0xC+4N      int32       removed_count
0x10+4N     int32       removed_node_id_1
0x14+4N     int32       removed_node_id_2
...
0x0+M                   other_data
...
```
There's no need to read or write the node entities yourself, simply read or write the ids.