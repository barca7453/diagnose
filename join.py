import diagnose as d
import time

d.start()
time.sleep(2)
db=d.DB("/home/jayendra/data/logs")
tcm = db.table("COMPLETED_MOPS")
tmh = db.table("MOPS_HISTORY")
tcm.eq_join(tmh,"meta_opid")

