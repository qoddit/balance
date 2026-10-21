module.exports = {
  version: "8.0.0",
  run: [{
    method: "shell.run",
    params: {
      message: "if [ ! -x ./build/balance ]; then echo 'Balance is not built. Run Install / Build first.'; exit 1; fi; ./build/balance",
      path: "."
    }
  }]
};
