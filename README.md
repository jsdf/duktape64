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

this will bring up a javascript REPL, into which you can type javascript expressions and receive the evaluated results
sometimes the > prompt gets lost, just press enter
to load and execute a javascript file from your local machine, use the command `.load [your js filename]`