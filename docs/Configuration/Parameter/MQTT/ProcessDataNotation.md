# Parameter: Process Data Notation

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Process Data Notation | ProcessDataNotation
| Default Value     | `JSON`              | `0`
| Input Options     | `JSON`<br>`Single Topics`<br>`JSON + Topics`| `0`<br>`1`<br>`2`


## Description

This parameter defines how the process data get provided.


### Input Options

| Input Option              | Description
|:---                       |:---
| `JSON`                    | All process related data points get provided as one topic in JSON notation.
| `Single Topics`           | Process related data get provided by multiple single topics, one for each data point.
| `JSON + Topics`           | Topic in JSON notation + single topics, one for each data point.


!!! Note
    Using Home Assistant discovery, topic with JSON notation is published even it's not selected with this parameter.

