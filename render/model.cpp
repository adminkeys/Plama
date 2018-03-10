#include "render/model.h"
#include "util.h"
#include <QPair>

using namespace std;

function<void(function<void(int)> &, int, int)> f0 = //
    [](function<void(int)> &f, int x, int y) {
        for (int i = 0; i < x; i++)
            for (int j = 0; j < y; j++) f(j * x + i);
    };

function<void(function<void(int)> &, int, int)> f1 = //
    [](function<void(int)> &f, int x, int y) {
        for (int i = x - 1; i >= 0; i--)
            for (int j = 0; j < y; j++) f(j * x + i);
    };

function<void(function<void(int)> &, int, int)> f2 = //
    [](function<void(int)> &f, int x, int y) {
        for (int i = 0; i < x; i++)
            for (int j = y - 1; j >= 0; j--) f(j * x + i);
    };

function<void(function<void(int)> &, int, int)> f3 = //
    [](function<void(int)> &f, int x, int y) {
        for (int i = x - 1; i >= 0; i--)
            for (int j = y - 1; j >= 0; j--) f(j * x + i);
    };

vector<function<void(function<void(int)> &, int, int)>> Model::indexFunc{
    f0, f0, f1, f1, f3, f3, f2, f2};

Model::Model() : indexT(8, vector<GLuint>()), indexL(8, vector<GLuint>()) {}
const vector<GLuint> &Model::getIndexT(int dir) const { return indexT[dir]; }
const vector<GLuint> &Model::getIndexL(int dir) const { return indexL[dir]; }
const vector<QVector3D> &Model::getPoint() const { return point; }
const vector<QVector3D> &Model::getNormal() const { return normal; }
const vector<QVector3D> &Model::getColor() const { return color; }
const vector<QVector3D> &Model::getPosition() const { return position; }

bool Model::setQuantity(SimQuantity &sq, float time, int step) {
    QVector2D extreme = sq.getExtreme();
    auto dataAt = [&](int dim = 0) {
        return Accessor<float>::gen(sq.getDataAt(time, dim), step, sq.getSizeData()[0]);
    };
    auto data = [&](const auto &d) { return Accessor<float>::gen(d, step); };

    switch (sq.getSizeData().size()) {
    case 0: genLine(data(sq.getTimes()), data(sq.getData()), extreme); return true;
    case 2:
        switch (sq.getDim()) {
        case 1: genHeight(dataAt(), extreme); return false;
        case 2: genVector(dataAt(0), dataAt(1), extreme); return false;
        }
        break;
    }
    return false;
}

void Model::genLine(DATA x, DATA y, QVector2D extreme) {
    if (checkSame(LINE, {x, y})) return; // TODO
    checkSize(x->sizeXI(), 0, (x->sizeXI() - 1) * 2);
    float xMin = x->get(0);
    float xMax = x->get(-1);
    float xDiff = xMax - xMin;
    float yMin = extreme.x();
    float yMax = extreme.y();
    float yDiff = yMax - yMin;

    if (yDiff == 0) {
        if (yMin == 0) {
            yMin = -1;
            yDiff = 2;
        } else {
            yMin *= 0.9;
            yMax *= 1.1;
            yDiff = yMax - yMin;
        }
    }

    for (int i = 0; i < x->sizeXI(); i++) {
        point[i] = QVector3D((x->get(i) - xMin) / xDiff, (y->get(i) - yMin) / yDiff, 0);
        normal[i] = QVector3D(0, 0, 1);
        color[i] = QVector3D(0, 0, 1);
        position[i] = point[i];
    }

    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < x->sizeXI() - 1; i++) {
            indexL[j][2 * i] = i;
            indexL[j][2 * i + 1] = i + 1;
        }
    }
}

void Model::genHeight(DATA data, QVector2D extreme) {
    if (checkSame(HEIGHT, {data})) return;
    int sxi = data->sizeXI();
    int syi = data->sizeYI();
    float sxf = data->sizeXF();
    float syf = data->sizeYF();
    checkSize((sxi - 1) * (syi - 1) * 9, (sxi - 1) * (syi - 1) * 24, 0);

    static vector<vector<GLuint>> order{
        {0, 4, 3, 1, 4, 0, 3, 4, 6, 6, 4, 7, 2, 4, 1, 5, 4, 2, 7, 4, 8, 8, 4, 5},
        {1, 4, 0, 0, 4, 3, 3, 4, 6, 6, 4, 7, 2, 4, 1, 5, 4, 2, 8, 4, 5, 7, 4, 8},
        {2, 4, 1, 5, 4, 2, 1, 4, 0, 0, 4, 3, 8, 4, 5, 7, 4, 8, 3, 4, 6, 6, 4, 7},
        {5, 4, 2, 2, 4, 1, 1, 4, 0, 0, 4, 3, 8, 4, 5, 7, 4, 8, 6, 4, 7, 3, 4, 6},
        {8, 4, 5, 7, 4, 8, 5, 4, 2, 2, 4, 1, 6, 4, 7, 3, 4, 6, 1, 4, 0, 0, 4, 3},
        {7, 4, 8, 8, 4, 5, 5, 4, 2, 2, 4, 1, 6, 4, 7, 3, 4, 6, 0, 4, 3, 1, 4, 0},
        {6, 4, 7, 3, 4, 6, 7, 4, 8, 8, 4, 5, 0, 4, 3, 1, 4, 0, 5, 4, 2, 2, 4, 1},
        {3, 4, 6, 6, 4, 7, 7, 4, 8, 8, 4, 5, 0, 4, 3, 1, 4, 0, 2, 4, 1, 5, 4, 2}};

    float height = extreme.y() - extreme.x();
    for (int y = 0; y < syi - 1; y++) {
        for (int x = 0; x < sxi - 1; x++) {
            float d[9];
            float px[3];
            float py[3];
            QVector3D p[9];

            int offsetPoint = (y * (sxi - 1) + x) * 9;

            if (height == 0) {
                for (int i = 0; i < 9; i++) d[i] = data->get(offsetPoint);
            } else {
                d[0] = data->get(x, y);
                d[2] = data->get(x + 1, y);
                d[6] = data->get(x, y + 1);
                d[8] = data->get(x + 1, y + 1);
                d[0] = (d[0] - extreme.x()) / height;
                d[2] = (d[2] - extreme.x()) / height;
                d[6] = (d[6] - extreme.x()) / height;
                d[8] = (d[8] - extreme.x()) / height;
                d[1] = (d[0] + d[2]) / 2;
                d[3] = (d[0] + d[6]) / 2;
                d[5] = (d[2] + d[8]) / 2;
                d[7] = (d[6] + d[8]) / 2;
                d[4] = (d[3] + d[5]) / 2;
            }

            for (int i = 0; i < 3; i++) {
                px[i] = (x + 0.5f * i) / sxf;
                py[i] = (y + 0.5f * i) / syf;
            }

            for (int yy = 0; yy < 3; yy++) {
                for (int xx = 0; xx < 3; xx++) {
                    int i = yy * 3 + xx;
                    p[i] = QVector3D(px[xx], py[yy], d[i]);
                }
            }

            for (int i = 0; i < 9; i++) point[offsetPoint + i] = p[i];

            normal[offsetPoint] = QVector3D::crossProduct(p[1] - p[0], p[4] - p[0]);
            normal[offsetPoint + 1] = QVector3D::crossProduct(p[2] - p[1], p[4] - p[1]);
            normal[offsetPoint + 2] = QVector3D::crossProduct(p[5] - p[2], p[4] - p[2]);
            normal[offsetPoint + 3] = QVector3D::crossProduct(p[0] - p[3], p[4] - p[3]);
            normal[offsetPoint + 4] = QVector3D(1, 1, 1);
            normal[offsetPoint + 5] = QVector3D::crossProduct(p[8] - p[5], p[4] - p[5]);
            normal[offsetPoint + 6] = QVector3D::crossProduct(p[3] - p[6], p[4] - p[6]);
            normal[offsetPoint + 7] = QVector3D::crossProduct(p[6] - p[7], p[4] - p[7]);
            normal[offsetPoint + 8] = QVector3D::crossProduct(p[7] - p[8], p[4] - p[8]);

            color[offsetPoint] = Gradient::HEIGHT_MAP.getColor((d[0] + d[4]) / 2);
            color[offsetPoint + 3] = color[offsetPoint];
            color[offsetPoint + 1] = Gradient::HEIGHT_MAP.getColor((d[2] + d[4]) / 2);
            color[offsetPoint + 2] = color[offsetPoint + 1];
            color[offsetPoint + 5] = Gradient::HEIGHT_MAP.getColor((d[8] + d[4]) / 2);
            color[offsetPoint + 8] = color[offsetPoint + 5];
            color[offsetPoint + 7] = Gradient::HEIGHT_MAP.getColor((d[6] + d[4]) / 2);
            color[offsetPoint + 6] = color[offsetPoint + 7];
            color[offsetPoint + 4] = Gradient::HEIGHT_MAP.getColor(0);

            position[offsetPoint] = (p[0] + p[1] + p[4]) / 3;
            position[offsetPoint + 1] = (p[1] + p[2] + p[4]) / 3;
            position[offsetPoint + 2] = (p[2] + p[5] + p[4]) / 3;
            position[offsetPoint + 3] = (p[0] + p[3] + p[4]) / 3;
            position[offsetPoint + 4] = p[4];
            position[offsetPoint + 5] = (p[5] + p[8] + p[4]) / 3;
            position[offsetPoint + 6] = (p[6] + p[3] + p[4]) / 3;
            position[offsetPoint + 7] = (p[6] + p[7] + p[4]) / 3;
            position[offsetPoint + 8] = (p[7] + p[8] + p[4]) / 3;
        }
    }

    for (int i = 0; i < 8; i++) {
        int cnt = 0;
        function<void(int)> func = [&](int j) {
            int offsetPoint = j * 9;
            for (int j = 0; j < 24; j++) indexT[i][cnt++] = order[i][j] + offsetPoint;
        };
        indexFunc[i](func, sxi - 1, syi - 1);
    }
}

void Model::genVector(
    Model::DATA dataX, Model::DATA dataY, QVector2D extreme, float ratio) {
    static vector<QPair<QVector2D, float>> polar;
    static float angle1 = PI * 13 / 12;
    static float angle2 = PI * 11 / 12;
    static vector<float> bufY;
    int sizeX = dataX->sizeXI();
    int sizeY = dataX->sizeYI();
    int sizeT = sizeX * sizeY;

    if (checkSame(VECTOR, {dataX, dataY})) return; // TODO
    checkSize(sizeX * sizeY * 5, sizeX * sizeY * 3, sizeX * sizeY * 2);
    polar.resize(sizeT);
    bufY.resize(sizeT);
    for (int i = 0; i < sizeT; i++) bufY[i] = dataY->get(i) * ratio;

    const DATA &convX = dataX;
    DATA convY = Accessor<float>::gen(bufY, 1, sizeX);

    QVector2D pnt;
    toPolar(convX->get(0), convY->get(0), pnt);
    float maxActual = extreme.y();
    float maxScaled = 0;

    for (int i = 0; i < sizeT; i++) {
        toPolar(convX->get(i), convY->get(i), pnt);
        QPair<QVector2D, float> &pair = polar[i];
        pair.first = pnt;
        pair.second = magnitude(dataX->get(i), dataY->get(i));
        if (pnt.x() > maxScaled) maxScaled = pnt.x();
    }
    int divs = sizeX > sizeY ? sizeX : sizeY;
    float toLenth = maxScaled == 0 ? 0 : 1 / maxScaled / divs;
    float toUnity = 1 / maxActual;
    float sizeL = 0.8 / (sizeX > sizeY ? sizeX + 1 : sizeY + 1);
    float sizeA = sizeL * cos(PI / 12);
    float diffX = 1.0 / (dataX->sizeXF() + 1);
    float diffY = 1.0 / (dataX->sizeYF() + 1);
    for (auto &i : polar) {
        i.first[0] *= toLenth;
        i.second *= toUnity;
    }
    QVector2D line0;
    QVector2D line1;
    QVector2D line2;
    for (int i = 0; i < sizeY; i++) {
        for (int j = 0; j < sizeX; j++) {
            int idx = i * sizeX + j;
            float offX = diffX * (j + 0.5);
            float offY = diffY * (i + 0.5);
            QVector2D &p = polar[idx].first;
            QVector3D base(offX, offY, 0);
            toCatsn(p.x(), p.y(), line0);
            QVector3D top = base + line0;
            point[idx * 5] = base;
            point[idx * 5 + 1] = top;
            toCatsn(sizeA, p.y(), line0);
            toCatsn(sizeL, p.y() + angle1, line1);
            toCatsn(sizeL, p.y() + angle2, line2);
            top = top + line0;
            point[idx * 5 + 2] = top;
            point[idx * 5 + 3] = top + line1;
            point[idx * 5 + 4] = top + line2;
            QVector3D c = Gradient::HEIGHT_MAP.getColor(polar[idx].second);
            for (int i = 0; i < 5; i++) color[idx * 5 + i] = c;
        }
    }
    for (size_t i = 0; i < normal.size(); i++) normal[i] = QVector3D(0, 0, 1);
    for (int i = 0; i < 8; i++) {
        int cntT = 0;
        int cntL = 0;
        function<void(int)> func = [&](int j) {
            int offset = j * 5;
            if (polar[j].first.x() == 0) return;
            for (int j = 0; j < 3; j++) indexT[i][cntT++] = offset + j + 2;
            for (int j = 0; j < 2; j++) indexL[i][cntL++] = offset + j;
        };
        indexFunc[i](func, sizeX, sizeY);
        indexT[i].resize(cntT);
        indexL[i].resize(cntL);
    }
}

bool Model::checkSame(Model::enumType type, std::vector<DATA> &&data) {
    bool ret = true;
    if (type != this->type)
        ret = false;
    else if (data.size() != this->data.size())
        ret = false;
    else
        for (size_t i = 0; i < data.size(); i++)
            if (data[i] != this->data[i]) ret = false;

    if (!ret) {
        this->type = type;
        this->data = std::move(data);
    }
    return ret;
}

void Model::checkSize(int point, int indexT, int indexL) {
    this->point.resize(point);
    color.resize(point);
    position.resize(point);
    normal.resize(point);

    for (auto &i : this->indexT) i.resize(indexT);
    for (auto &i : this->indexL) i.resize(indexL);
}

QPair<float, float> Model::getExtreme(const float *data, int total) {
    float max = *data;
    float min = *data;
    for (int i = 1; i < total; i++) {
        float f = data[i];
        if (f > max) max = f;
        if (f < min) min = f;
    }
    return QPair<float, float>(min, max);
}
