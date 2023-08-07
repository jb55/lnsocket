# lnsocket test

We use `pyln-testing` python library to write the tests.

The Go program `getinfo.go` we use to test `lnsocket` depends on
`tidwall/gjson` library.

Before, we run the tests, we set up a virtual environment, install
`pyln-testing` Python library and `gjson` Go package:

    python -m venv .venv
    source .venv/bin/activate
    pip install pyln-testing
    go get github.com/tidwall/gjson

Now we can run the tests in `.venv` environment like this:

    pytest test_lnsocket.py
