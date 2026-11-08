"""Times a few heavy EnlngDB scripts with the given engine. Usage: python bench.py <enlngdb.exe> [scale]"""
import subprocess, sys, tempfile, time, os

exe = sys.argv[1]
N = int(sys.argv[2]) if len(sys.argv) > 2 else 20000


def run(name, script, check=None):
    d = tempfile.mkdtemp()
    open(os.path.join(d, 'b.enlngdb'), 'w').write('type enlngdb\nuse database bench\n' + script)
    t0 = time.time()
    try:
        r = subprocess.run([exe, 'b.enlngdb'], cwd=d, capture_output=True, text=True, timeout=600)
        out = r.stdout + r.stderr
    except subprocess.TimeoutExpired:
        out = 'TIMEOUT'
    dt = time.time() - t0
    ok = 'ERROR' not in out and 'TIMEOUT' not in out and (check is None or check in out)
    print('%-44s %8.2f s  %s' % (name, dt, 'ok' if ok else 'PROBLEM: ' + out[-300:].replace('\n', ' | ')))


rows = ', '.join('(%d, "name%d", %d)' % (i, i, i % 100) for i in range(1, N + 1))
run('insert %d rows, id key, one statement' % N,
    'create table t with id integer primary key, name text, grp integer\ninsert into t values %s\ncount records in t\n' % rows, '| %d ' % N)

singles = '\n'.join('insert into t values (%d, "name%d", %d)' % (i, i, i % 100) for i in range(1, N // 4 + 1))
run('insert %d rows, one statement each' % (N // 4),
    'create table t with id integer primary key, name text, grp integer\n%s\ncount records in t\n' % singles, '| %d ' % (N // 4))

run('insert %d rows into a unique + auto table' % N,
    'create table u with id integer primary key auto_increment, email text unique\ninsert into u (email) values %s\ncount records in u\n'
    % ', '.join('("e%d@x.org")' % i for i in range(N)), '| %d ' % N)

parent = ', '.join('(%d)' % i for i in range(1, N + 1))
child = ', '.join('(%d, %d)' % (i, (i % N) + 1) for i in range(1, N + 1))
run('insert %d rows that reference a parent' % N,
    'create table p with id integer primary key\ninsert into p values %s\ncreate table c with cid integer primary key, pid integer references p(id)\ninsert into c values %s\ncount records in c\n' % (parent, child), '| %d ' % N)

run('join %d x %d on a key' % (N, N),
    'create table a with id integer primary key, v integer\ninsert into a values %s\ncreate table b with bid integer primary key, aid integer\ninsert into b values %s\nfind count(*) from a join b on a.id is b.aid\n'
    % (', '.join('(%d, %d)' % (i, i) for i in range(1, N + 1)), ', '.join('(%d, %d)' % (i, i) for i in range(1, N + 1))), '| %d ' % N)

run('in (subquery) over %d rows' % N,
    'create table a with id integer primary key\ninsert into a values %s\ncreate table b with x integer\ninsert into b values %s\nfind count(*) from a where id in (find x from b)\n'
    % (parent, parent), '| %d ' % N)

run('where id is X, 2000 lookups in %d rows' % N,
    'create table t with id integer primary key, v integer\ninsert into t values %s\n%s\n'
    % (', '.join('(%d, %d)' % (i, i) for i in range(1, N + 1)), '\n'.join('find v from t where id is %d' % (i * 7 % N + 1) for i in range(2000))))
