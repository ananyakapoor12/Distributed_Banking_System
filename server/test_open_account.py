import socket
import struct
import uuid

HOST = '127.0.0.1'
PORT = 8014

SOCK = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
SOCK.settimeout(3)

STATUS = {
    0:  'SUCCESS',
    2:  'MALFORMED_REQUEST',
    4:  'DUPLICATE_REQUEST',
    10: 'AUTH_FAILED',
    11: 'ACCOUNT_NOT_FOUND',
    20: 'INSUFFICIENT_BALANCE',
    22: 'ACCOUNT_ALREADY_CLOSED',
    32: 'INVALID_AMOUNT',
}

# ─── Helpers ──────────────────────────────────────────────────────────────────

def send_recv(buf):
    SOCK.sendto(bytes(buf), (HOST, PORT))
    resp, _ = SOCK.recvfrom(1024)
    return resp

def parse_response(resp):
    # msg_type(1) + request_id(16) + opcode(1) + status(1) = 19 bytes header
    status = resp[18]
    body   = resp[19:]
    return status, body

def status_str(code):
    return STATUS.get(code, f'UNKNOWN({code})')

def header(opcode_byte, request_id=None):
    buf = bytearray()
    buf += b'\x00'                                            # MessageType::REQUEST
    buf += request_id if request_id else uuid.uuid4().bytes  # 16-byte request ID
    buf += opcode_byte
    return buf

# ─── Request builders ─────────────────────────────────────────────────────────
# Field order matches handlers.cpp parse order, NOT protocol.h comments.

def build_open_account(name, password, currency, balance, request_id=None):
    # handlers.cpp: password → currency → balance → name
    buf = header(b'\x01', request_id)
    buf += password.encode().ljust(8, b'\x00')   # fixed 8-byte password
    buf += bytes([currency])                     # currency byte
    buf += struct.pack('>f', balance)            # 4-byte float
    name_bytes = name.encode()
    buf += struct.pack('>H', len(name_bytes))    # 2-byte length prefix
    buf += name_bytes
    return buf

def build_close_account(account_num, name, password):
    # handlers.cpp: account_num → password → name
    buf = header(b'\x02')
    buf += struct.pack('>I', account_num)        # 4-byte account number
    buf += password.encode().ljust(8, b'\x00')   # fixed 8-byte password
    name_bytes = name.encode()
    buf += struct.pack('>H', len(name_bytes))    # 2-byte length prefix
    buf += name_bytes
    return buf

def build_deposit(account_num, name, password, currency, amount, request_id=None):
    # handlers.cpp: account_num → password → currency → amount → name
    buf = header(b'\x03', request_id)
    buf += struct.pack('>I', account_num)        # 4-byte account number
    buf += password.encode().ljust(8, b'\x00')   # fixed 8-byte password
    buf += bytes([currency])                     # currency byte
    buf += struct.pack('>f', amount)             # 4-byte float amount
    name_bytes = name.encode()
    buf += struct.pack('>H', len(name_bytes))    # 2-byte length prefix
    buf += name_bytes
    return buf

def build_withdraw(account_num, name, password, currency, amount):
    # handlers.cpp: account_num → password → currency → amount → name
    buf = header(b'\x04')
    buf += struct.pack('>I', account_num)        # 4-byte account number
    buf += password.encode().ljust(8, b'\x00')   # fixed 8-byte password
    buf += bytes([currency])                     # currency byte
    buf += struct.pack('>f', amount)             # 4-byte float amount
    name_bytes = name.encode()
    buf += struct.pack('>H', len(name_bytes))    # 2-byte length prefix
    buf += name_bytes
    return buf

def build_transfer(sender_account_num, sender_name, sender_password,
                   receiver_account_num, receiver_name, currency, amount):
    # handlers.cpp: sender_account_num → sender_password → receiver_account_num
    #               → amount → currency → sender_name → receiver_name
    buf = header(b'\x06')
    buf += struct.pack('>I', sender_account_num)
    buf += sender_password.encode().ljust(8, b'\x00')
    buf += struct.pack('>I', receiver_account_num)
    buf += struct.pack('>f', amount)
    buf += bytes([currency])
    sender_bytes = sender_name.encode()
    buf += struct.pack('>H', len(sender_bytes))
    buf += sender_bytes
    receiver_bytes = receiver_name.encode()
    buf += struct.pack('>H', len(receiver_bytes))
    buf += receiver_bytes
    return buf

def build_check_balance(account_num, name, password):
    # handlers.cpp: account_num → password → name
    buf = header(b'\x07')
    buf += struct.pack('>I', account_num)        # 4-byte account number
    buf += password.encode().ljust(8, b'\x00')   # fixed 8-byte password
    name_bytes = name.encode()
    buf += struct.pack('>H', len(name_bytes))    # 2-byte length prefix
    buf += name_bytes
    return buf

# ─── Tests ────────────────────────────────────────────────────────────────────

def test_open_account(name, password, currency, balance):
    print(f"\n=== test_open_account ({name}) ===")
    buf = build_open_account(name=name, password=password, currency=currency, balance=balance)
    resp = send_recv(buf)
    status, body = parse_response(resp)
    print(f"  status : {status_str(status)}")
    if status == 0:
        account_num = struct.unpack('>I', body[:4])[0]
        print(f"  account_num : {account_num}")
        return account_num
    return None

# ── Deposit ───────────────────────────────────────────────────────────────────

def test_deposit_success(account_num):
    """Scenario 1: deposit into a valid account → SUCCESS, returns new balance."""
    print("\n=== test_deposit — success ===")
    buf = build_deposit(account_num, name='Alice', password='pass1234', currency=1, amount=250.0)
    resp = send_recv(buf)
    status, body = parse_response(resp)
    print(f"  status : {status_str(status)}")
    if status == 0:
        new_balance = struct.unpack('>f', body[:4])[0]
        print(f"  new_balance : {new_balance:.2f}  currency : {body[4]}")
    assert status == 0, f"Expected SUCCESS, got {status_str(status)}"

def test_deposit_wrong_password(account_num):
    """Scenario 2: deposit with wrong password → AUTH_FAILED."""
    print("\n=== test_deposit — wrong password ===")
    buf = build_deposit(account_num, name='Alice', password='wrongpwd', currency=1, amount=100.0)
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 10, f"Expected AUTH_FAILED(10), got {status_str(status)}"

def test_deposit_account_not_found():
    """Scenario 3: deposit into a non-existent account → ACCOUNT_NOT_FOUND."""
    print("\n=== test_deposit — account not found ===")
    buf = build_deposit(999999, name='Nobody', password='pass1234', currency=1, amount=100.0)
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 11, f"Expected ACCOUNT_NOT_FOUND(11), got {status_str(status)}"

def test_deposit_invalid_amount(account_num):
    """Scenario 4: deposit zero amount → INVALID_AMOUNT."""
    print("\n=== test_deposit — invalid amount (zero) ===")
    buf = build_deposit(account_num, name='Alice', password='pass1234', currency=1, amount=0.0)
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 32, f"Expected INVALID_AMOUNT(32), got {status_str(status)}"

# ── Withdraw ──────────────────────────────────────────────────────────────────

def test_withdraw_success(account_num):
    """Scenario 1: withdraw a valid amount → SUCCESS, returns new balance."""
    print("\n=== test_withdraw — success ===")
    buf = build_withdraw(account_num, name='Alice', password='pass1234', currency=1, amount=100.0)
    resp = send_recv(buf)
    status, body = parse_response(resp)
    print(f"  status : {status_str(status)}")
    if status == 0:
        new_balance = struct.unpack('>f', body[:4])[0]
        print(f"  new_balance : {new_balance:.2f}  currency : {body[4]}")
    assert status == 0, f"Expected SUCCESS, got {status_str(status)}"

def test_withdraw_wrong_password(account_num):
    """Scenario 2: withdraw with wrong password → AUTH_FAILED."""
    print("\n=== test_withdraw — wrong password ===")
    buf = build_withdraw(account_num, name='Alice', password='wrongpwd', currency=1, amount=100.0)
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 10, f"Expected AUTH_FAILED(10), got {status_str(status)}"

def test_withdraw_account_not_found():
    """Scenario 3: withdraw from a non-existent account → ACCOUNT_NOT_FOUND."""
    print("\n=== test_withdraw — account not found ===")
    buf = build_withdraw(999999, name='Nobody', password='pass1234', currency=1, amount=100.0)
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 11, f"Expected ACCOUNT_NOT_FOUND(11), got {status_str(status)}"

def test_withdraw_insufficient_balance(account_num):
    """Scenario 4: withdraw more than the balance → INSUFFICIENT_BALANCE."""
    print("\n=== test_withdraw — insufficient balance ===")
    buf = build_withdraw(account_num, name='Alice', password='pass1234', currency=1, amount=999999.0)
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 20, f"Expected INSUFFICIENT_BALANCE(20), got {status_str(status)}"

def test_withdraw_invalid_amount(account_num):
    """Scenario 5: withdraw zero amount → INVALID_AMOUNT."""
    print("\n=== test_withdraw — invalid amount (zero) ===")
    buf = build_withdraw(account_num, name='Alice', password='pass1234', currency=1, amount=0.0)
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 32, f"Expected INVALID_AMOUNT(32), got {status_str(status)}"

# ── Transfer ──────────────────────────────────────────────────────────────────

def test_transfer_success(alice_id, bob_id):
    """Scenario 1: transfer from Alice to Bob → SUCCESS, returns sender's new balance."""
    print("\n=== test_transfer — success ===")
    buf = build_transfer(alice_id, 'Alice', 'pass1234', bob_id, 'Bob', currency=1, amount=50.0)
    resp = send_recv(buf)
    status, body = parse_response(resp)
    print(f"  status : {status_str(status)}")
    if status == 0:
        new_balance = struct.unpack('>f', body[:4])[0]
        print(f"  sender new_balance : {new_balance:.2f}  currency : {body[4]}")
    assert status == 0, f"Expected SUCCESS, got {status_str(status)}"

def test_transfer_wrong_password(alice_id, bob_id):
    """Scenario 2: transfer with wrong sender password → AUTH_FAILED."""
    print("\n=== test_transfer — wrong password ===")
    buf = build_transfer(alice_id, 'Alice', 'wrongpwd', bob_id, 'Bob', currency=1, amount=50.0)
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 10, f"Expected AUTH_FAILED(10), got {status_str(status)}"

def test_transfer_sender_not_found(bob_id):
    """Scenario 3: sender account does not exist → ACCOUNT_NOT_FOUND."""
    print("\n=== test_transfer — sender not found ===")
    buf = build_transfer(999999, 'Nobody', 'pass1234', bob_id, 'Bob', currency=1, amount=50.0)
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 11, f"Expected ACCOUNT_NOT_FOUND(11), got {status_str(status)}"

def test_transfer_insufficient_balance(alice_id, bob_id):
    """Scenario 4: transfer more than sender's balance → INSUFFICIENT_BALANCE."""
    print("\n=== test_transfer — insufficient balance ===")
    buf = build_transfer(alice_id, 'Alice', 'pass1234', bob_id, 'Bob', currency=1, amount=999999.0)
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 20, f"Expected INSUFFICIENT_BALANCE(20), got {status_str(status)}"

# ── Check Balance ─────────────────────────────────────────────────────────────

def test_check_balance_success(account_num):
    """Scenario 1: check balance of a valid account → SUCCESS, returns balance."""
    print("\n=== test_check_balance — success ===")
    buf = build_check_balance(account_num, name='Alice', password='pass1234')
    resp = send_recv(buf)
    status, body = parse_response(resp)
    print(f"  status : {status_str(status)}")
    if status == 0:
        balance = struct.unpack('>f', body[:4])[0]
        print(f"  balance : {balance:.2f}  currency : {body[4]}")
    assert status == 0, f"Expected SUCCESS, got {status_str(status)}"

def test_check_balance_wrong_password(account_num):
    """Scenario 2: check balance with wrong password → AUTH_FAILED."""
    print("\n=== test_check_balance — wrong password ===")
    buf = build_check_balance(account_num, name='Alice', password='wrongpwd')
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 10, f"Expected AUTH_FAILED(10), got {status_str(status)}"

def test_check_balance_account_not_found():
    """Scenario 3: check balance of a non-existent account → ACCOUNT_NOT_FOUND."""
    print("\n=== test_check_balance — account not found ===")
    buf = build_check_balance(999999, name='Nobody', password='pass1234')
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  status : {status_str(status)}")
    assert status == 11, f"Expected ACCOUNT_NOT_FOUND(11), got {status_str(status)}"

# ── At-most-once cache ────────────────────────────────────────────────────────

def test_duplicate_request_cached(account_num):
    """Send the same request ID twice.
    First response should be SUCCESS; second should be DUPLICATE_REQUEST (4),
    confirming the server returned the cached reply without re-executing.
    Also verifies the account balance is only changed once.
    """
    print("\n=== test_duplicate_request — cache hit ===")
    rid = uuid.uuid4().bytes   # fixed request ID reused for both sends
    buf = build_deposit(account_num, name='Alice', password='pass1234',
                        currency=1, amount=10.0, request_id=rid)

    # First send — should execute and return SUCCESS
    resp1 = send_recv(buf)
    status1, body1 = parse_response(resp1)
    print(f"  1st send status : {status_str(status1)}")
    assert status1 == 0, f"Expected SUCCESS on first send, got {status_str(status1)}"
    balance_after_first = struct.unpack('>f', body1[:4])[0]
    print(f"  balance after 1st send : {balance_after_first:.2f}")

    # Second send — same bytes, same request ID
    resp2 = send_recv(buf)
    status2, _ = parse_response(resp2)
    print(f"  2nd send status : {status_str(status2)}")
    assert status2 == 4, f"Expected DUPLICATE_REQUEST(4) on second send, got {status_str(status2)}"

    # The cached response body must be identical to the first response
    assert resp1 == resp2, "Cached response differs from original — server did not return cached reply"
    print("  cached response matches original response")

# ── Close Account ─────────────────────────────────────────────────────────────

def test_close_account_success(account_num, name, password):
    """Scenario 1: close a valid account with correct credentials."""
    print(f"\n=== test_close_account — success ({name}) ===")
    buf = build_close_account(account_num, name=name, password=password)
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  account_num : {account_num}")
    print(f"  status      : {status_str(status)}")
    assert status == 0, f"Expected SUCCESS, got {status_str(status)}"

def test_close_account_wrong_password(account_num):
    """Scenario 2: close with wrong password → AUTH_FAILED."""
    print("\n=== test_close_account — wrong password ===")
    buf = build_close_account(account_num, name='Alice', password='wrongpwd')
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  account_num : {account_num}")
    print(f"  status      : {status_str(status)}")
    assert status == 10, f"Expected AUTH_FAILED(10), got {status_str(status)}"

def test_close_account_not_found():
    """Scenario 3: close a non-existent account → ACCOUNT_NOT_FOUND."""
    print("\n=== test_close_account — account not found ===")
    fake_id = 999999
    buf = build_close_account(fake_id, name='Nobody', password='pass1234')
    resp = send_recv(buf)
    status, _ = parse_response(resp)
    print(f"  account_num : {fake_id}")
    print(f"  status      : {status_str(status)}")
    assert status == 11, f"Expected ACCOUNT_NOT_FOUND(11), got {status_str(status)}"

# ─── Main ─────────────────────────────────────────────────────────────────────

if __name__ == '__main__':
    # Open two accounts — Alice for most tests, Bob as transfer receiver
    alice_id = test_open_account('Alice', 'pass1234', currency=1, balance=500.0)
    bob_id   = test_open_account('Bob',   'secret12', currency=1, balance=100.0)
    if alice_id is None or bob_id is None:
        print("open_account failed; aborting")
        exit(1)

    # deposit
    test_deposit_wrong_password(alice_id)
    test_deposit_account_not_found()
    test_deposit_invalid_amount(alice_id)
    test_deposit_success(alice_id)           # Alice: 500 + 250 = 750

    # withdraw
    test_withdraw_wrong_password(alice_id)
    test_withdraw_account_not_found()
    test_withdraw_invalid_amount(alice_id)
    test_withdraw_insufficient_balance(alice_id)
    test_withdraw_success(alice_id)          # Alice: 750 - 100 = 650

    # transfer
    test_transfer_wrong_password(alice_id, bob_id)
    test_transfer_sender_not_found(bob_id)
    test_transfer_insufficient_balance(alice_id, bob_id)
    test_transfer_success(alice_id, bob_id)  # Alice: 650 - 50 = 600, Bob: 100 + 50 = 150

    # at-most-once cache
    test_duplicate_request_cached(alice_id)      # Alice: 600 + 10 = 610 (only once)

    # check balance
    test_check_balance_wrong_password(alice_id)
    test_check_balance_account_not_found()
    test_check_balance_success(alice_id)

    # close (error cases first, then close both accounts)
    test_close_account_wrong_password(alice_id)
    test_close_account_not_found()
    test_close_account_success(alice_id, 'Alice', 'pass1234')
    test_close_account_success(bob_id,   'Bob',   'secret12')

    print("\nAll tests passed.")
