#pragma once

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

#include "fontclass.h"
#include "fontshaderclass.h"

class TextClass
{
private:
    struct SentenceType
    {
        ID3D11Buffer* vertexBuffer;
        ID3D11Buffer* indexBuffer;
        int vertexCount, indexCount, maxLength;
        float red, green, blue;
    };

    struct VertexType
    {
        XMFLOAT3 position;
        XMFLOAT2 texture;
    };

public:
    TextClass();
    TextClass(const TextClass& other);
    ~TextClass();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, HWND hwnd,
        int screenWidth, int screenHeight, XMMATRIX baseViewMatrix);
    void Shutdown();

    bool Render(ID3D11DeviceContext* deviceContext, XMMATRIX worldMatrix, XMMATRIX orthoMatrix);

    bool SetMousePosition(int mouseX, int mouseY, ID3D11DeviceContext* deviceContext);

    bool SetSceneInfo(
        int fps, int cpu,
        int polyCount, int objCount,
        int screenWidth, int screenHeight,
        ID3D11DeviceContext* deviceContext
    );

    // ====== 여기 추가: 타이틀 전용 ======
    bool SetTitleLine(int lineIndex, const char* text, ID3D11DeviceContext* deviceContext);
    void SetTitleVisible(bool visible) { m_titleVisible = visible; }

private:
    bool InitializeSentence(SentenceType** sentence, int maxLength, ID3D11Device* device);
    bool UpdateSentence(SentenceType* sentence, const char* text, int positionX, int positionY,
        float red, float green, float blue, ID3D11DeviceContext* deviceContext);
    void ReleaseSentence(SentenceType** sentence);
    bool RenderSentence(ID3D11DeviceContext* deviceContext, SentenceType* sentence,
        XMMATRIX worldMatrix, XMMATRIX orthoMatrix);

private:
    FontClass* m_Font;
    FontShaderClass* m_FontShader;
    int m_screenWidth, m_screenHeight;
    XMMATRIX m_baseViewMatrix;

    SentenceType* m_sentence1;
    SentenceType* m_sentence2;

    // ====== 여기 추가: 타이틀 문장 배열 ======
    static const int TITLE_MAX_LINES = 5;
    SentenceType* m_titleSentences[TITLE_MAX_LINES];
    bool m_titleVisible;
};
