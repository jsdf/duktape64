# duktape64

- setup n64 sdk under wine:
  
    make sure the `wine` command is on your PATH

    install the n64 sdk into the root of the wine C: drive so you have eg.
    ```
    C:\ultra
    C:\nintendo
    ```
- git clone
- in repo dir:
  - `npm install`
  - `./build_exegcc.sh`
  - connect everdrive64 v3
  - `./deploy.sh`