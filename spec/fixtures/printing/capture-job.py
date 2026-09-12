#!/usr/bin/env python3
"""ippeveprinter command: atomically capture a received job, excluding secrets."""

import json
import os
from pathlib import Path
import sys

document = Path(sys.argv[1])
attributes = (
    "CONTENT_TYPE", "IPP_JOB_NAME", "IPP_MEDIA_COL", "IPP_MEDIA_COL_DEFAULT",
    "IPP_COPIES", "IPP_COPIES_DEFAULT", "IPP_SIDES", "IPP_SIDES_DEFAULT",
    "IPP_PRINT_COLOR_MODE", "IPP_MULTIPLE_DOCUMENT_HANDLING",
)
job = {key: os.environ[key] for key in attributes if key in os.environ}
with document.open("rb") as stream:
    job["document"] = {
        "bytes": document.stat().st_size,
        "prefix": stream.read(8).decode("latin1"),
    }
target = document.with_suffix(".json")
temporary = target.with_suffix(".json.tmp")
temporary.write_text(json.dumps(job), encoding="utf8")
temporary.replace(target)
