#pragma once

#include "ofMain.h"

namespace
{
	void toggleFullscreen(size_t width, size_t height)
	{
		ofToggleFullscreen();
		if (!(ofGetWindowMode() == OF_FULLSCREEN))
		{
			ofSetWindowShape(width, height);
			auto pos = ofVec2f(ofGetScreenWidth() - width, ofGetScreenHeight() - height) * 0.5f;
			ofSetWindowPosition(pos.x, pos.y);
		}
	}

	ofRectangle getCenteredRect(int srcWidth, int srcHeight, int otherWidth, int otherHeight, bool isFill = true)
	{
		ofRectangle other(0, 0, otherWidth, otherHeight);
		ofRectangle result; result.setFromCenter(other.getCenter(), srcWidth, srcHeight);
		float scaleBy = other.getHeight() / result.getHeight();
		auto aspectAspect = result.getAspectRatio() / other.getAspectRatio();
		if ((isFill && aspectAspect <= 1.0f) || (!isFill && aspectAspect >= 1.0f))
			scaleBy = other.getWidth() / result.getWidth();
		result.scaleFromCenter(scaleBy);
		return result;
	}


	void drawRectangle(float x, float y, float w, float h)
	{
		static ofVbo vbo;
		static vector<ofVec4f> vertices(4);
		vertices[0].set(x, y, 0, 1);
		vertices[1].set(x + w, y, 0, 1);
		vertices[2].set(x + w, y + h, 0, 1);
		vertices[3].set(x, y + h, 0, 1);
		static vector<ofFloatColor> colors(4);
		colors.assign(4, ofGetStyle().color);
		if (!vbo.getIsAllocated())
		{
			vector<ofVec2f> texCoords = { ofVec2f(0, 0), ofVec2f(1, 0), ofVec2f(1, 1), ofVec2f(0, 1) };
			vector<ofVec3f> normals(4, ofVec3f(0, 0, 1));
			vector<ofIndexType> indices = { 0, 2, 1, 0, 3, 2 };
			vbo.setVertexData(&vertices[0].x, 4, vertices.size(), GL_DYNAMIC_DRAW);
			vbo.setColorData(&colors[0].r, colors.size(), GL_DYNAMIC_DRAW);
			vbo.setTexCoordData(&texCoords[0].x, texCoords.size(), GL_STATIC_DRAW);
			vbo.setNormalData(&normals[0].x, normals.size(), GL_STATIC_DRAW);
			vbo.setIndexData(&indices[0], indices.size(), GL_STATIC_DRAW);
		}
		else
		{
			vbo.updateVertexData(&vertices[0].x, vertices.size());
			vbo.updateColorData(&colors[0].r, colors.size());
		}
		vbo.drawElements(GL_TRIANGLES, vbo.getNumIndices());
	}
	void drawRectangle(ofVec2f pos, float w, float h) { drawRectangle(pos.x, pos.y, w, h); }
	void drawRectangle(const ofRectangle& rect) { drawRectangle(rect.x, rect.y, rect.width, rect.height); }

}

