# Parameter: Input Debounce Time

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Input Debounce Time | inputdebouncetime
| Default Value     | `200`               | `200`
| Input Options     | `0 .. 5000`         | `0 .. 5000`
| Unit              | Milliseconds        | Milliseconds



## Description

Input debounce time to avoid to many capture events due to any signal flicker 
of e.g. while pressing a push button (only for GPIO input modes)


!!! Note
    Debouncing is removing unwanted input noise from buttons, switches or other user input.