from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import geodispatch as gd

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# ── P1 — Ragini ───────────────────────────────────────────────

class QueryRequest(BaseModel):
    lat: float
    lon: float

class KNNRequest(BaseModel):
    lat: float
    lon: float
    k: int = 3


@app.post("/query-nearest")
def query_nearest(body: QueryRequest):
    result = gd.kd_nearest(body.lat, body.lon)
    if result.get("id") == -1:
        return {"error": "No facilities available"}
    return {"facility": result}


@app.post("/query-knn")
def query_knn(body: KNNRequest):
    results = gd.kd_knn(body.lat, body.lon, body.k)
    return {"k": body.k, "facilities": results}


# ── other endpoints go below (Sachi, Sanat will add theirs) ───

##git add src/kd.h src/kd.c python/api.py
##git commit -m "P1 complete: kd-tree build, NN, kNN, endpoints"
