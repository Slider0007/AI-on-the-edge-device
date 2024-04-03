[Overview](_OVERVIEW.md) 

## MQTT API: Process Status

The following topics get published during the `Publish To MQTT` state **every cycle**.

!!! __Note__: 
    These topics are not retained because they get updated on a regulary basis.

- Format: `[MainTopic]/process/status/[Topic]`
- Example: `watermeter/process/status/process_state`

- Notation: Single Topics

| Topic                       | Description                 | Output
|:----------------------------|:----------------------------|:--------------
| `process_status`            | Process Status<br><br>Possible States:<br>- `Processing (Automatic)`:  Timer-controlled automatic processing<br>- `Processing (Triggered Only)`: Manual triggered processing only<br>- `Not Processing / Not Ready`: Initializing / Initialization failed | `Processing (Automatic)`
| `process_state`             | Actual Process State        | `Idle - Waiting for Autostart`
| `process_interval`          | Automatic Process Interval [min] | `2.0`
| `process_time`              | Process Time [sec]          | `25`
| `process_error`             | Process Error State<br>- Error definition: Process error with cycle abortion, e.g. alignment failed<br>- Deviation definition: Process deviation with cycle continuation, e.g. rate limit exceeded<br><br>Possible States:<br>- `0`: No error/deviation<br>- `-1`: One error occured<br>- `-2`: Multiple process errors in a row<br>- `1`: One process deviation occured<br>- `2`: Multiple process deviations in a row | `0`
| `cycle_counter`             | Process Cycle Counter       | `64`


