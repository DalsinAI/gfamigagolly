/* Run100.rexx: Golly steps the pattern 100 times, then gives the population.
   rx Run100.rexx   (Golly running, a pattern open) */
OPTIONS RESULTS
IF ~SHOW('P', 'GOLLY.1') THEN DO
    SAY 'Golly is not running.'
    EXIT 5
END
ADDRESS 'GOLLY.1'
DO 100
    'STEP'
END
'GETGEN'
gen = RESULT
'GETPOP'
SAY 'Generation' gen', population' RESULT
