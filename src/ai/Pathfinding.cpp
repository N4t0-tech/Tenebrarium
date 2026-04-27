// Pathfinding BFS — calcula el primer paso del camino más corto de 'from' a 'to'.
// Retorna 'from' si ya está en destino o si no existe camino.
// Usa movimiento en 4 direcciones (no diagonal) para evitar cruzar esquinas.
// La reconstrucción del camino sigue prev[] desde 'to' hasta encontrar 'from'.

#include "Pathfinding.hpp"
#include <vector>
#include <queue>

namespace AI {

Position bfsStep(Position from, Position to, const Map& map)
{
    if (from.x == to.x && from.y == to.y)
        return from;

    const int W = map.width(), H = map.height();
    std::vector<int>      dist(W * H, -1);
    std::vector<Position> prev(W * H, {-1, -1});
    std::queue<Position>  q;
    auto idx = [W](int x, int y) { return y * W + x; };

    dist[idx(from.x, from.y)] = 0;
    q.push(from);

    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1,  0, 0};
    bool found = false;

    while (!q.empty() && !found) {
        auto [x, y] = q.front();
        q.pop();
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], ny = y + dy[d];
            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
            if (dist[idx(nx, ny)] != -1)    continue;
            if (!map.isWalkable(nx, ny))    continue;
            dist[idx(nx, ny)] = dist[idx(x, y)] + 1;
            prev[idx(nx, ny)] = {x, y};
            if (nx == to.x && ny == to.y) { found = true; break; }
            q.push({nx, ny});
        }
    }

    if (!found) return from;

    Position cur = to;
    while (prev[idx(cur.x, cur.y)].x != from.x ||
           prev[idx(cur.x, cur.y)].y != from.y) {
        Position p = prev[idx(cur.x, cur.y)];
        if (p.x == -1) return from;
        cur = p;
    }
    return cur;
}

} // namespace AI
