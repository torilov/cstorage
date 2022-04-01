import os
import signal
import subprocess
import time
import inspect


def stdout(proc):
    return ''.join(map(lambda x: x.decode(), proc.stdout.readlines()))


def stderr(proc):
    return ''.join(map(lambda x: x.decode(), proc.stderr.readlines()))


def test_single_connect_disconnect():
    cstorage = subprocess.Popen(["./cstorage", "8080", "servdir"],
                                stdout=subprocess.PIPE)
    time.sleep(0.02)

    nc = subprocess.Popen('printf "." | nc -q 0.01 localhost 8080',
                          shell=True,
                          stdout=subprocess.PIPE)
    nc.wait()

    cstorage.kill()
    cstorage_stdout = stdout(cstorage)

    assert "new connection" in cstorage_stdout
    assert "disconnected" in cstorage_stdout
    print(inspect.stack()[0][3], "ok")


def test_concurrent_connects():
    cstorage = subprocess.Popen(["./cstorage", "8080", "servdir"],
                                stdout=subprocess.PIPE)
    time.sleep(0.01)

    for _ in range(100):
        subprocess.Popen('printf "." | nc localhost 8080',
                         shell=True,
                         stderr=subprocess.DEVNULL)

    time.sleep(0.1)

    nc = subprocess.Popen('printf "." | nc -q 0.01 localhost 8080',
                          shell=True,
                          stdout=subprocess.PIPE)
    nc.wait()

    time.sleep(0.1)

    cstorage.kill()
    cstorage_stdout = stdout(cstorage)

    assert "disconnected" in cstorage_stdout
    print(inspect.stack()[0][3], "ok")


def test_concurrent_connects2():
    cstorage = subprocess.Popen(["./cstorage", "8080", "servdir"],
                                stdout=subprocess.PIPE)
    time.sleep(0.01)

    for _ in range(100):
        subprocess.Popen('cat /dev/urandom | nc localhost 8080',
                         shell=True,
                         stderr=subprocess.DEVNULL)

    time.sleep(0.1)

    nc = subprocess.Popen('printf "." | nc -q 0.01 localhost 8080',
                          shell=True,
                          stdout=subprocess.PIPE)
    nc.wait()

    time.sleep(0.1)

    cstorage.kill()
    cstorage_stdout = stdout(cstorage)

    assert "[connection 101] disconnected" in cstorage_stdout
    print(inspect.stack()[0][3], "ok")


def test_incorrect_header():
    cstorage = subprocess.Popen(["./cstorage", "8080", "servdir"],
                                stdout=subprocess.PIPE)
    time.sleep(0.01)

    nc = subprocess.Popen('printf "INCORRECT HEADER" | nc localhost 8080',
                          shell=True,
                          stderr=subprocess.PIPE)
    nc.wait()

    cstorage.kill()
    cstorage_stdout = stdout(cstorage)

    assert "incorrect header" in cstorage_stdout

    nc_stderr = stderr(nc)
    assert "" == nc_stderr
    print(inspect.stack()[0][3], "ok")


def test_save_too_big_file():
    cstorage = subprocess.Popen(["./cstorage", "8080", "servdir"],
                                stdout=subprocess.PIPE)
    time.sleep(0.01)

    nc = subprocess.Popen('printf "SET\nFFFFFFFF\n" | nc localhost 8080',
                          shell=True,
                          stdout=subprocess.PIPE)
    nc.wait()

    cstorage.kill()
    cstorage_stdout = stdout(cstorage)

    assert "file is too big" in cstorage_stdout
    print(inspect.stack()[0][3], "ok")


def test_save_file():
    cstorage = subprocess.Popen(["./cstorage", "8080", "servdir"],
                                stdout=subprocess.PIPE)
    time.sleep(0.01)

    nc = subprocess.Popen(
        'printf "SET\n0000000F\n0123456789ABCDE" | nc localhost 8080',
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL)
    nc.wait()

    cstorage.kill()
    cstorage_stdout = stdout(cstorage)

    assert "saved" in cstorage_stdout

    nc_stdout = stdout(nc)

    cat = subprocess.Popen(f'cat ./servdir/{nc_stdout}',
                           shell=True,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.DEVNULL)
    cat.wait()
    cat_stdout = stdout(cat)

    assert cat_stdout == "0123456789ABCDE"
    print(inspect.stack()[0][3], "ok")


def test_concurrent_saves():
    cstorage = subprocess.Popen(["./cstorage", "8080", "servdir"],
                                stdout=subprocess.PIPE)
    time.sleep(0.01)

    ncs = []

    for i in range(100):
        subprocess.Popen('printf "." | nc localhost 8080',
                         shell=True,
                         stderr=subprocess.DEVNULL)

        ncs.append(
            subprocess.Popen(
                'bash -c \'rand=$(uuidgen) && file=$(printf "SET\n00000025\n$rand\n" | nc localhost 8080) && diff -w <(printf "$rand") "./servdir/$file" && echo "ok"\'',
                shell=True,
                stdout=subprocess.PIPE))

    for nc in ncs:
        nc.wait()
        nc_stdout = stdout(nc)
        assert "ok" in nc_stdout

    cstorage.kill()
    time.sleep(0.01)
    print(inspect.stack()[0][3], "ok")


def test_read_incorrect_file():
    cstorage = subprocess.Popen(["./cstorage", "8080", "servdir"],
                                stdout=subprocess.PIPE)
    time.sleep(0.01)

    nc = subprocess.Popen(
        'printf "GET\nNOTEXISTNOTEXISTNOTEXISTNOTEXISTNOTEXIST" | nc localhost 8080',
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL)
    nc.wait()

    cstorage.kill()
    cstorage_stdout = stdout(cstorage)

    assert "not exists" in cstorage_stdout
    print(inspect.stack()[0][3], "ok")


def test_write_and_read_file():
    cstorage = subprocess.Popen(["./cstorage", "8080", "servdir"],
                                stdout=subprocess.PIPE)
    time.sleep(0.01)

    nc_write = subprocess.Popen(
        'printf "SET\n0000000C\nHello World!" | nc localhost 8080',
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL)
    nc_write.wait()
    filename = stdout(nc_write).strip()

    nc_read = subprocess.Popen(
        f'printf "GET\n{filename}" | nc localhost 8080',
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL)
    nc_read.wait()
    content = stdout(nc_read).strip()

    cstorage.kill()

    assert content == "Hello World!"
    print(inspect.stack()[0][3], "ok")


def test_with_client():
    cstorage = subprocess.Popen(["./cstorage", "8080", "servdir"],
                                stdout=subprocess.PIPE)
    time.sleep(0.01)

    set_client = subprocess.Popen(
        './cstorage-client 127.0.0.1 8080 set example.txt',
        shell=True,
        stdout=subprocess.PIPE)
    set_client.wait()
    file_id = stdout(set_client).strip()

    get_client = subprocess.Popen(
        f'./cstorage-client 127.0.0.1 8080 get {file_id}',
        shell=True,
        stdout=subprocess.PIPE)
    get_client.wait()
    file_content = stdout(get_client).strip()

    assert "electronic typesetting" in file_content

    cstorage.kill()

    cstorage_stdout = stdout(cstorage)

    print(inspect.stack()[0][3], "ok")


test_single_connect_disconnect()
test_concurrent_connects()
test_concurrent_connects2()
test_incorrect_header()
test_save_too_big_file()
test_save_file()
test_concurrent_saves()
test_read_incorrect_file()
test_write_and_read_file()
test_with_client()

