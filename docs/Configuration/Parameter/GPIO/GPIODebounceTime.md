# Parameter: GPIO Debounce Time

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | GPIO PWM Frequency  | IOx: 3. parameter
| Default Value     | `200`               | `200`
| Input Options     | `0 .. 5000`         | `0 .. 5000`
| Unit              | Milliseconds        | Milliseconds



## Description

GPIO debounce time to avoid to many capture events due to any signal flicker 
of e.g. while pressing a push button (only for GPIO input modes)


!!! Note
    Debouncing is removing unwanted input noise from buttons, switches or other user input.