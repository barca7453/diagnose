import diagnose as d
import time
d.start()
time.sleep(2)
db=d.DB("/home/jayendra/data/logs")
time.sleep(10)
t=db.table("STARGATE_TRACE")
t.query("*","")

