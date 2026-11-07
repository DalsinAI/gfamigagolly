/* arexx.rexx: Golly's ARexx port (GOLLY.1). Start Golly first, with
 * RexxMast running; give the drawer of a pattern (default the Patterns
 * drawer beside Golly). Prints PASS or FAIL for each check, then QUITs it.
 * Copyright (c) 2026 AmiCradle Team */
OPTIONS RESULTS
PARSE ARG pat
IF pat = '' THEN pat = 'Patterns/Life/Guns/gosper-glider-gun.rle'
bad = 0
ADDRESS 'GOLLY.1'

'VERSION'
CALL check 'VERSION', rc = 0 & LEFT(result, 6) = 'Golly '
'HELP'
CALL check 'HELP', rc = 0 & POS('OPEN', result) > 0 & POS('GETPOP', result) > 0
'OPEN' pat
CALL check 'OPEN', rc = 0
'GETRULE'
CALL check 'GETRULE', rc = 0 & result ~= ''
'GETGEN'
g0 = result
CALL check 'GETGEN', rc = 0 & DATATYPE(g0, 'W')
'GETPOP'
p0 = result
CALL check 'GETPOP', rc = 0 & DATATYPE(p0, 'W') & p0 > 0
'STEP'
CALL check 'STEP', rc = 0
'GETGEN'
CALL check 'STEP moves on', result > g0
'RESET'
'GETGEN'
CALL check 'RESET', rc = 0 & result = g0
'OPEN NoSuchDrawer/none.rle'
CALL check 'OPEN missing', rc = 10
'NOSUCH'
CALL check 'unknown', rc >= 10
'QUIT'
CALL check 'QUIT', rc = 0
IF bad = 0 THEN SAY 'golly arexx: ALL PASS'
ELSE SAY 'golly arexx:' bad 'FAILED'
EXIT bad

check: PARSE ARG what, ok
IF ok THEN SAY 'PASS' what
ELSE DO
  SAY 'FAIL' what '(rc' rc')'
  bad = bad + 1
END
RETURN
