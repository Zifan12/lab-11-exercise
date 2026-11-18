#ifndef SPRITE_MANAGER_H
#define SPRITE_MANAGER_H

using uint = unsigned int;

#include "basic_transform.h"
#include "texture_atlas.h"
#include "vector_util.h"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#define GL_SILENCE_DEPRECATION  // silences macOS OpenGL deprecation warnings

#define glCheck(expr)                                                                              \
    expr;                                                                                          \
    if (int e = glGetError()) {                                                                    \
        std::cerr << "OpenGL error: " << e << " in file " __FILE__ << ", line number " << __LINE__ \
                  << '\n';                                                                         \
        abort();                                                                                   \
    }

using SpriteID = size_t;

class SpriteManager {
public:
    SpriteManager(const sf::Vector2u& atlasSize, uint padding)
        : mVertices(sf::PrimitiveType::Triangles),
          mNumSprites{0},
          mAtlas(atlasSize, padding),
          mShader(std::filesystem::path{"main.vert"}, std::filesystem::path{"main.frag"}) {
        // TODO
        mPositionAttribLocation = glGetAttribLocation(mShader.getNativeHandle(), "position");
        mTexCoordsAttribLocation = glGetAttribLocation(mShader.getNativeHandle(), "texCoords");
        mSpriteNumberAttribLocation =
            glGetAttribLocation(mShader.getNativeHandle(), "spriteNumber");
        mTextureUniformLocation = glGetUniformLocation(mShader.getNativeHandle(), "texture");
        mTransformsUniformLocation = glGetUniformLocation(mShader.getNativeHandle(), "transforms");
        mNumTransformsUniformLocation =
            glGetUniformLocation(mShader.getNativeHandle(), "numTransforms");

        glGenBuffers(1, &mVerticesVBO);
        glGenBuffers(1, &mSpriteNumberVBO);
        glGenTextures(1, &mTransformsTexture);
    }

    ~SpriteManager() {
        glDeleteBuffers(1, &mVerticesVBO);
        glDeleteBuffers(1, &mSpriteNumberVBO);
        glDeleteTextures(1, &mTransformsTexture);
    }

    // TODO
    TextureID registerTexture(const std::string& filepath) {
        TextureID id = mAtlas.registerTexture(filepath);
        return id;
    };

    // TODO
    SpriteID createSprite(TextureID textureID) {
        sf::Vector2f size = sf::Vector2f(
            mAtlas.getSizePixels(textureID));  // sprite width/height in pixels (= world units)
        sf::FloatRect uv = mAtlas.getBoundsUV(
            textureID);  // this texture's rectangle inside the atlas, as fractions 0..1

        sf::Vertex bottomLeft{{0, 0}, sf::Color::White, {uv.position}};
        sf::Vertex bottomRight{
            {size.x, 0}, sf::Color::White, {uv.position.x + uv.size.x, uv.position.y}};
        sf::Vertex topRight{{size.x, size.y},
                            sf::Color::White,
                            {uv.position.x + uv.size.x, uv.position.y + uv.size.y}};
        sf::Vertex topLeft{
            {0, size.y}, sf::Color::White, {uv.position.x, uv.position.y + uv.size.y}};

        //
        mVertices.append(bottomLeft);
        mVertices.append(bottomRight);
        mVertices.append(topRight);
        mVertices.append(topLeft);
        mVertices.append(bottomLeft);
        mVertices.append(topRight);

        mTransforms.emplace_back();

        SpriteID id = mNumSprites;

        for (int k = 0; k < 6; k++) {
            mSpriteNumbers.push_back(static_cast<float>(mNumSprites));
        }

        mNumSprites += 1;

        return id;
    };

    // TODO
    void setTexture(SpriteID sprite, TextureID newTexture) {
        sf::Vector2f size = sf::Vector2f(mAtlas.getSizePixels(newTexture));
        sf::FloatRect uv = mAtlas.getBoundsUV(newTexture);

        sf::Vertex bottomLeft{{0, 0}, sf::Color::White, {uv.position}};
        sf::Vertex bottomRight{
            {size.x, 0}, sf::Color::White, {uv.position.x + uv.size.x, uv.position.y}};
        sf::Vertex topRight{{size.x, size.y},
                            sf::Color::White,
                            {uv.position.x + uv.size.x, uv.position.y + uv.size.y}};
        sf::Vertex topLeft{
            {0, size.y}, sf::Color::White, {uv.position.x, uv.position.y + uv.size.y}};

        size_t first = 6 * sprite;
        mVertices[first + 0] = bottomLeft;
        mVertices[first + 1] = bottomRight;
        mVertices[first + 2] = topRight;
        mVertices[first + 3] = topLeft;
        mVertices[first + 4] = bottomLeft;
        mVertices[first + 5] = topRight;
    };

    // TODO
    void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        sf::VertexArray copy = mVertices;

        for (size_t i = 0; i < copy.getVertexCount(); i++) {
            auto sprite = i / 6;
            copy[i].position = (mTransforms[sprite].applyPoint(copy[i].position));
        }

        states.texture = &mAtlas.getTexture();
        states.shader = &mShader;
        target.draw(copy, states);
    };

    // TODO
    void drawGL() {
        glUseProgram(mShader.getNativeHandle());
        glBindBuffer(GL_ARRAY_BUFFER, mVerticesVBO);
        glBufferData(GL_ARRAY_BUFFER, mVertices.getVertexCount() * sizeof(sf::Vertex),
                     &mVertices[0], GL_DYNAMIC_DRAW);

        glVertexAttribPointer(mPositionAttribLocation, 2, GL_FLOAT, GL_FALSE, sizeof(sf::Vertex),
                              (void*)0);
        glEnableVertexAttribArray(mPositionAttribLocation);

        glVertexAttribPointer(mTexCoordsAttribLocation, 2, GL_FLOAT, GL_FALSE, sizeof(sf::Vertex),
                              (void*)12);
        glEnableVertexAttribArray(mTexCoordsAttribLocation);

        glBindBuffer(GL_ARRAY_BUFFER, mSpriteNumberVBO);
        glBufferData(GL_ARRAY_BUFFER, mSpriteNumbers.size() * sizeof(float), &mSpriteNumbers[0],
                     GL_DYNAMIC_DRAW);

        glVertexAttribPointer(mSpriteNumberAttribLocation, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(mSpriteNumberAttribLocation);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, mTransformsTexture);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F_ARB, mTransforms.size(), 0, GL_RGBA, GL_FLOAT,
                     mTransforms.data());
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glActiveTexture(GL_TEXTURE1);
        sf::Texture::bind(&mAtlas.getTexture());

        glUniform1i(mTransformsUniformLocation, 0);
        glUniform1i(mTextureUniformLocation, 1);
        glUniform1i(mNumTransformsUniformLocation, static_cast<GLint>(mTransforms.size()));

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mVertices.getVertexCount()));
    };

    // TODO
    BasicTransform& getTransform(SpriteID id) { return mTransforms[id]; }

    // TODO
    const TextureAtlas& getAtlas() const { return mAtlas; };

private:
    sf::VertexArray mVertices;
    std::vector<BasicTransform> mTransforms;
    size_t mNumSprites;
    TextureAtlas mAtlas;
    mutable sf::Shader mShader;

    // Use the below only in Batched rendering (part 2):
    std::vector<float> mSpriteNumbers;

    GLint mTextureUniformLocation;
    GLint mTransformsUniformLocation;
    GLint mNumTransformsUniformLocation;

    GLint mPositionAttribLocation;
    GLint mTexCoordsAttribLocation;
    GLint mSpriteNumberAttribLocation;

    GLuint mVerticesVBO;
    GLuint mSpriteNumberVBO;
    GLuint mTransformsTexture;
};

#endif  // SPRITE_MANAGER_H
