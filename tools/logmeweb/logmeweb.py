from pathlib import Path
from typing import Optional

from fastapi import FastAPI
from fastapi import HTTPException
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
import uvicorn

from control_client import SendControlCommand
from discovery import DiscoverProcesses


BASE_DIR = Path(__file__).resolve().parent

app = FastAPI(title="logmeweb")
app.mount("/static", StaticFiles(directory=str(BASE_DIR / "static")), name="static")


class CommandRequest(BaseModel):
  host: str
  port: int
  protocol: str = "http"
  password: Optional[str] = ""
  command: str
  format: str = "text"


@app.get("/")
async def Index():
  return FileResponse(str(BASE_DIR / "templates" / "index.html"))


@app.get("/api/discovery")
async def ApiDiscovery():
  return DiscoverProcesses()


@app.post("/api/command")
async def ApiCommand(request: CommandRequest):
  try:
    return SendControlCommand(
      request.host
      , request.port
      , request.protocol
      , request.password or ""
      , request.command
      , request.format
    )
  except Exception as e:
    raise HTTPException(status_code=500, detail=str(e))


if __name__ == "__main__":
  uvicorn.run(
    "logmeweb:app"
    , host="127.0.0.1"
    , port=8080
    , reload=False
  )
