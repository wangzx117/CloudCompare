//##########################################################################
//#                                                                        #
//#                            CLOUDCOMPARE                                #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
//#                                                                        #
//##########################################################################

//Inspired from ccColorScaleEditorWidget by Richard Steffen (LGPL 2.1)

#include "ccColorScaleEditorWidget.h"

//Qt
#include <QColorDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>

//System
#include <assert.h>

static const int DEFAULT_SLIDER_SYMBOL_SIZE = 8;
static const int DEFAULT_MARGIN = DEFAULT_SLIDER_SYMBOL_SIZE/2+1;
static const int DEFAULT_TEXT_MARGIN = 2;

/*******************************/
/*** ColorScaleElementSlider ***/
/*******************************/

ColorScaleElementSlider::ColorScaleElementSlider(double value/*=0.0*/, QColor color/*=Qt::black*/, QWidget* parent/*=0*/, Qt::Orientation orientation/*=Qt::Horizontal*/)
	: QWidget(parent)
	, ccColorScaleElement(value,color)
	, m_selected(false)
	, m_orientation(orientation)
{
	if (m_orientation == Qt::Horizontal)
		setFixedSize(DEFAULT_SLIDER_SYMBOL_SIZE, 2*DEFAULT_SLIDER_SYMBOL_SIZE);
	else
		setFixedSize(2*DEFAULT_SLIDER_SYMBOL_SIZE, DEFAULT_SLIDER_SYMBOL_SIZE);
}

void ColorScaleElementSlider::paintEvent(QPaintEvent* e)
{
	QPainter painter(this);

	painter.setPen(m_selected ? Qt::red : Qt::black);
	painter.setBrush(m_color);

	QRect box(0,0,DEFAULT_SLIDER_SYMBOL_SIZE-1,DEFAULT_SLIDER_SYMBOL_SIZE-1);
	QPolygon pointyHead;
	if (m_orientation == Qt::Horizontal)
	{
		box.moveTop(DEFAULT_SLIDER_SYMBOL_SIZE-1);
		pointyHead << QPoint(0,DEFAULT_SLIDER_SYMBOL_SIZE-1) << QPoint(DEFAULT_SLIDER_SYMBOL_SIZE/2,0) << QPoint(DEFAULT_SLIDER_SYMBOL_SIZE-1,DEFAULT_SLIDER_SYMBOL_SIZE-1);
	}
	else
	{
		box.moveLeft(DEFAULT_SLIDER_SYMBOL_SIZE-1);
		pointyHead << QPoint(DEFAULT_SLIDER_SYMBOL_SIZE-1,0) << QPoint(0,DEFAULT_SLIDER_SYMBOL_SIZE/2) << QPoint(DEFAULT_SLIDER_SYMBOL_SIZE-1,DEFAULT_SLIDER_SYMBOL_SIZE-1);
	}

	painter.drawRect(box);
	painter.drawPolygon(pointyHead, Qt::OddEvenFill);
}

/********************************/
/*** ColorScaleElementSliders ***/
/********************************/

void ColorScaleElementSliders::addSlider(ColorScaleElementSlider* slider)
{
	assert(slider);

	if (slider)
	{
		push_back(slider);
		sort();
	}
}

void ColorScaleElementSliders::sort()
{
	qSort(begin(), end(), ColorScaleElementSlider::IsSmaller);
}

void ColorScaleElementSliders::clear()
{
	while (!isEmpty())
	{
		back()->setParent(0);
		delete back();
		pop_back();
	}
}

void ColorScaleElementSliders::removeAt(int i)
{
	if (i<0 || i>=size())
	{
		assert(false);
		return;
	}

	ColorScaleElementSlider* slider = at(i);
	if (slider)
	{
		slider->setParent(0);
		delete slider;
		slider = 0;
	}
	
	QList<ColorScaleElementSlider*>::removeAt(i);
}

int ColorScaleElementSliders::selected() const
{
	for (int i=0; i<size(); ++i)
		if (at(i)->isSelected())
			return i;

	return -1;
}

int ColorScaleElementSliders::indexOf(ColorScaleElementSlider* slider)
{
	for (int i=0; i<size(); ++i)
		if (at(i) == slider)
			return i;

	return -1;
}

/**********************/
/*** ColorBarWidget ***/
/**********************/

ColorBarWidget::ColorBarWidget(SharedColorScaleElementSliders sliders, QWidget* parent/*=0*/, Qt::Orientation orientation/*=Qt::Horizontal*/)
	: ColorScaleEditorBaseWidget(sliders, orientation, DEFAULT_MARGIN, parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setContentsMargins(0,0,0,0);
	setMinimumSize(DEFAULT_MARGIN*3,DEFAULT_MARGIN*3);
}

void ColorBarWidget::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		QRect contentRect = contentsRect();
		if (m_orientation == Qt::Horizontal)
			contentRect.adjust(m_margin,0,-m_margin,0);
		else
			contentRect.adjust(0,m_margin,0,-m_margin);

		if (contentRect.contains(e->pos(), true))
		{
			double relativePos = -1.0;
			if (m_orientation == Qt::Horizontal)
			{
				relativePos = (double)(e->pos().x()-contentRect.left())/(double)contentRect.width();
			}
			else
			{
				relativePos = (double)(e->pos().y()-contentRect.top())/(double)contentRect.height();
			}

			emit pointClicked(relativePos);
		}
	}
}

void ColorBarWidget::paintEvent(QPaintEvent* e)
{
	if (m_sliders && m_sliders->size()>=2)
	{
		QPainter painter(this);
		painter.setPen(Qt::black);

		QRect contentRect = contentsRect();
		if (m_orientation == Qt::Horizontal)
			contentRect.adjust(m_margin,0,-m_margin,-1);
		else
			contentRect.adjust(0,m_margin,-1,-m_margin);

		double minVal = m_sliders->front()->getValue();
		double maxVal = m_sliders->back()->getValue();
		double range  = maxVal-minVal;
		if (range == 0.0)
			range = 1.0;

		//color gradient
		{
			QLinearGradient gradient;
			if (m_orientation == Qt::Horizontal)
				gradient = QLinearGradient(0, 0, contentRect.width(), 0);
			else
				gradient = QLinearGradient(0, 0, 0, contentRect.height());

			//fill gradient with sliders
			{
				for (int i=0; i<m_sliders->size(); i++)
				{
					double relativePos = (m_sliders->at(i)->getValue()-minVal)/range;
					gradient.setColorAt(relativePos, m_sliders->at(i)->getColor());
				}
			}

			painter.fillRect(contentRect, gradient);
			painter.drawRect(contentRect);
		}

		//draw a line for each slider position (apart from the first and the last one)
		{
			QPoint A = contentRect.topLeft();
			QPoint B = contentRect.bottomRight();

			for (int i=0; i<m_sliders->size(); i++)
			{
				double relativePos = (m_sliders->at(i)->getValue()-minVal)/range;
				if (m_orientation == Qt::Horizontal)
				{
					int pos = contentRect.left() + (int)(relativePos * (double)contentRect.width());
					A.setX(pos);
					B.setX(pos);
				}
				else
				{
					int pos = contentRect.top() + (int)(relativePos * (double)contentRect.height());
					A.setY(pos);
					B.setY(pos);
				}
		
				painter.drawLine(A,B);
			}
		}
	}

	QWidget::paintEvent(e);
}

/*********************/
/*** SlidersWidget ***/
/*********************/

SlidersWidget::SlidersWidget(SharedColorScaleElementSliders sliders, QWidget* parent/*=0*/, Qt::Orientation orientation/*=Qt::Horizontal*/)
	: ColorScaleEditorBaseWidget(sliders,orientation,DEFAULT_MARGIN,parent)
{
	setContentsMargins(0,0,0,0);
	if (m_orientation == Qt::Horizontal)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		setMinimumSize(0,2*DEFAULT_SLIDER_SYMBOL_SIZE);
	}
	else
	{
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		setMinimumSize(2*DEFAULT_SLIDER_SYMBOL_SIZE,0);
	}
}

void SlidersWidget::select(int index)
{
	assert(m_sliders);

	//look for previously selected slider
	int activeSliderIndex = m_sliders->selected();

	if (index == activeSliderIndex) //nothing to do
		return;

	//deselect old one (if any)
	if (activeSliderIndex >= 0)
		m_sliders->at(activeSliderIndex)->setSelected(false);

	//select new one (if any)
	if (index >= 0)
		m_sliders->at(index)->setSelected(true);

	emit sliderSelected(index);
}

ColorScaleElementSlider* SlidersWidget::addNewSlider(double relativePos, double value, QColor color)
{
	select(-1); //be sure to deselect any selected slider before modifying the global set contents!

	ColorScaleElementSlider* slider = new ColorScaleElementSlider(value, color, this, m_orientation);

	m_sliders->addSlider(slider);
	
	int pos = (int)((double)length() * relativePos);

	if (m_orientation == Qt::Horizontal)
	{
		pos += DEFAULT_MARGIN - slider->width()/2;
		slider->move(pos,0);
	}
	else
	{
		pos += DEFAULT_MARGIN - slider->height()/2;
		slider->move(0,pos);
	}
	slider->setVisible(true);

	return slider;
}

void SlidersWidget::resizeEvent(QResizeEvent* e)
{
	updateAllSlidersPos();
}

void SlidersWidget::updateAllSlidersPos()
{
	if (!m_sliders || m_sliders->size() < 2)
		return;

	double minVal = m_sliders->front()->getValue();
	double maxVal = m_sliders->back()->getValue();
	double range  = maxVal-minVal;
	if (range == 0.0)
		range = 1.0;

	int rectLength = length();

	for (ColorScaleElementSliders::iterator it = m_sliders->begin(); it != m_sliders->end(); ++it)
	{
		ColorScaleElementSlider* slider = *it;
		int pos = (int)((slider->getValue() - minVal)/range * (double)rectLength);

		if (m_orientation == Qt::Horizontal)
		{
			pos += DEFAULT_MARGIN - slider->width()/2;
			slider->move(pos,0);
		}
		else
		{
			pos += DEFAULT_MARGIN - slider->height()/2;
			slider->move(0,pos);
		}
	}
}

void SlidersWidget::updateSliderPos(int index)
{
	if (!m_sliders || m_sliders->size() < 2 || index < 0)
		return;

	ColorScaleElementSlider* slider = m_sliders->at(index);

	double minVal = m_sliders->front()->getValue();
	double maxVal = m_sliders->back()->getValue();
	double range  = maxVal-minVal;

	int pos = (range == 0 ? (int)((slider->getValue() - minVal)/range * (double)length()) : 0);

	if (m_orientation == Qt::Horizontal)
	{
		pos += DEFAULT_MARGIN - slider->width()/2;
		slider->move(pos,0);
	}
	else
	{
		pos += DEFAULT_MARGIN - slider->height()/2;
		slider->move(0,pos);
	}
}

void SlidersWidget::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		if (!m_sliders || m_sliders->size() < 2)
			return;

		for (int i=0; i<m_sliders->size(); i++)
		{
			QRect rect = m_sliders->at(i)->geometry();
			if (rect.contains(e->pos(), true))
			{
				select(i);
				e->accept();
				break;
			}
		}
	}
}

void SlidersWidget::mouseMoveEvent(QMouseEvent* e)
{
	if (!m_sliders || m_sliders->size() <= 2)
		return;

	int pos = (m_orientation == Qt::Horizontal ? e->pos().x() : e->pos().y());
	double relativePos = (double)(pos-DEFAULT_MARGIN)/(double)length();

	if (relativePos > 0.0 && relativePos < 1.0)
	{
		int activeSliderIndex = m_sliders->selected();

		if (activeSliderIndex > 0 && activeSliderIndex+1 < m_sliders->size()) //first and last sliders can't move!
		{
			ColorScaleElementSlider* slider = m_sliders->at(activeSliderIndex);
			assert(slider && slider->isSelected());

			if (m_orientation == Qt::Horizontal)
				slider->move(pos - slider->width()/2, 0);
			else
				slider->move(0,pos - slider->height()/2);

			double newVal = m_sliders->front()->getValue() + relativePos*(m_sliders->back()->getValue()-m_sliders->front()->getValue());
			slider->setValue(newVal);

			m_sliders->sort();

			emit sliderModified(activeSliderIndex);

			e->accept();

			//update();
		}
	}
}

//void SlidersWidget::mouseReleaseEvent(QMouseEvent* e)
//{
//}

void SlidersWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		for (int i=0; i< m_sliders->size(); i++)
		{
			QRect rect = m_sliders->at(i)->geometry();
			if (rect.contains(e->pos(), true))
			{
				select(i);

				ColorScaleElementSlider* slider = m_sliders->at(i);
				assert(slider && slider->isSelected());

				//spawn a color choosing dialog?
				QColor newColor = QColorDialog::getColor(m_sliders->at(i)->getColor(),this);
				if (newColor.isValid() && newColor != slider->getColor())
				{
					slider->setColor(newColor);
					emit sliderModified(i);
				}

				break;
			}
		}
	}
}

/*************************/
/*** SliderLabelWidget ***/
/*************************/

SliderLabelWidget::SliderLabelWidget(SharedColorScaleElementSliders sliders, QWidget* parent/*=0*/, Qt::Orientation orientation/*=Qt::Horizontal*/)
	: ColorScaleEditorBaseWidget(sliders,orientation,DEFAULT_MARGIN,parent)
	, m_textColor(Qt::black)
	, m_precision(6)
{
	setContentsMargins(0,0,0,0);
}

void SliderLabelWidget::paintEvent(QPaintEvent* e)
{
	if (m_sliders)
	{
		QPainter painter(this);

		QFont font = painter.font();
		font.setPixelSize(8);
		painter.setFont(font);

		painter.setPen(m_textColor);
		painter.setBrush(m_textColor);

		QFontMetrics fm(font);

		if (m_orientation == Qt::Horizontal)
		{
			int labelHeight = fm.height()+DEFAULT_TEXT_MARGIN;

			//adjust height if necessary
			setMinimumSize(0,labelHeight);

			for (int i=0; i<m_sliders->size(); i++)
			{
				int pos = m_sliders->at(i)->pos().x();
				
				double val = m_sliders->at(i)->getValue();
				QString label = QString::number(val, 'f', m_precision);
				int labelWidth = fm.width(label);
				if (pos+labelWidth > width())
					pos -= (labelWidth - m_sliders->at(i)->width());
				
				painter.drawText(pos, labelHeight, label);
			}
		}
		else
		{
			//adjust width if necessary
			{
				QString firstLabel = QString::number(m_sliders->first()->getValue(), 'f', m_precision);
				QString lastLabel = QString::number(m_sliders->last()->getValue(), 'f', m_precision);
				int labelWidth = std::max(fm.width(firstLabel),fm.width(lastLabel))+2*DEFAULT_TEXT_MARGIN;
				setMinimumSize(labelWidth,0);
			}
			// draw the text for vertical orientation
			for (int i=0; i<m_sliders->size(); i++)
			{
				int pos = m_sliders->at(i)->pos().y();

				double val = m_sliders->at(i)->getValue();
				QString label = QString::number(val, 'f', m_precision);
				
				painter.drawText(DEFAULT_TEXT_MARGIN, pos + m_sliders->at(i)->height(), label);
			}
		}
	}

	QWidget::paintEvent(e);
}

/********************************/
/*** ccColorScaleEditorWidget ***/
/********************************/

ccColorScaleEditorWidget::ccColorScaleEditorWidget(QWidget* parent/*=0*/, Qt::Orientation orientation/*=Qt::Horizontal*/)
	: ColorScaleEditorBaseWidget(SharedColorScaleElementSliders(new ColorScaleElementSliders), orientation, 0, parent)
{
	//size and margin
	setMinimumSize(40,40);
	setContentsMargins(0,0,0,0);

	//layout
	setLayout(m_orientation == Qt::Horizontal ? static_cast<QLayout*>(new QVBoxLayout()) : static_cast<QLayout*>(new QHBoxLayout()));
	layout()->setMargin(0);
	layout()->setSpacing(0);
	layout()->setContentsMargins(0,0,0,0);

	//color bar
	{
		m_colorBarWidget = new ColorBarWidget(m_sliders,parent,orientation);
		m_colorBarWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		m_colorBarWidget->setContentsMargins(0,0,0,0);
		layout()->addWidget(m_colorBarWidget);

		connect(m_colorBarWidget, SIGNAL(pointClicked(double)), this, SLOT(onPointClicked(double)));
	}

	//sliders widget
	{
		m_slidersWidget = new SlidersWidget(m_sliders,parent,orientation);
		m_slidersWidget->setContentsMargins(0,0,0,0);
		layout()->addWidget(m_slidersWidget);

		//add default min and max elements
		m_slidersWidget->addNewSlider(0.0,0.0,Qt::blue);
		m_slidersWidget->addNewSlider(1.0,1.0,Qt::red);

		connect(m_slidersWidget, SIGNAL(sliderModified(int)), this, SLOT(onSliderModified(int)));
		connect(m_slidersWidget, SIGNAL(sliderSelected(int)), this, SLOT(onSliderSelected(int)));
	}

	//Labels widget
	{
		m_labelsWidget = new SliderLabelWidget(m_sliders,parent,orientation);
		if (m_orientation == Qt::Horizontal)
		{
			m_labelsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
			m_labelsWidget->setFixedHeight(12); //FIXME: add const
		}
		else
		{
			m_labelsWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
			m_labelsWidget->setFixedWidth(12); //FIXME: add const
		}
		layout()->addWidget(m_labelsWidget);
		//m_labelsWidget->setVisible(true);
	}
}

ccColorScaleEditorWidget::~ccColorScaleEditorWidget()
{
	//Qt will take care of its sibliings!
}

void ccColorScaleEditorWidget::onPointClicked(double relativePos)
{
	assert(relativePos>= 0.0 && relativePos <= 1.0);
	assert(m_colorBarWidget && m_slidersWidget);

	if (!m_sliders)
		return;

	double minVal = m_sliders->front()->getValue();
	double maxVal = m_sliders->back()->getValue();
	double value = minVal + relativePos*(maxVal-minVal);

	//look first if this position corresponds to an already existing slider
	double maxDist = 2.0*value/(double)m_colorBarWidget->length();
	for (int i=01; i<m_sliders->size(); ++i)
	{
		if (abs(m_sliders->at(i)->getValue() - value) < maxDist)
		{
			m_slidersWidget->select(i);
			return;
		}
	}

	//TODO: we should add it with the corresponding color!
	ColorScaleElementSlider* slider = m_slidersWidget->addNewSlider(relativePos, value, Qt::white);

	if (slider)
	{
		int pos = m_sliders->indexOf(slider);
		if (pos >= 0)
			m_slidersWidget->select(pos);
	}

	update();
}

void ccColorScaleEditorWidget::onSliderModified(int sliderIndex)
{
	if (sliderIndex<0)
	{
		assert(false);
		return;
	}

	if (m_colorBarWidget)
		m_colorBarWidget->update();
	if (m_slidersWidget)
		m_slidersWidget->update();
	if (m_labelsWidget)
		m_labelsWidget->update();

	emit stepModified(sliderIndex);
}

void ccColorScaleEditorWidget::onSliderSelected(int sliderIndex)
{
	if (m_slidersWidget)
		m_slidersWidget->update();

	emit stepSelected(sliderIndex);
}

void ccColorScaleEditorWidget::importColorScale(ccColorScale::Shared scale)
{
	m_sliders->clear();

	if (scale)
	{
		for (int i=0; i<scale->stepCount(); ++i)
		{
			double value = scale->step(i).getValue();
			const QColor& color = scale->step(i).getColor();
			m_slidersWidget->addNewSlider(scale->getRelativePosition(value),value,color);
		}
	}

	update();
}

ccColorScale::Shared ccColorScaleEditorWidget::exportColorScale() const
{
	ccColorScale::Shared scale(new ccColorScale("unknwon"));
	
	for (int i=0; i<m_sliders->size(); ++i)
		scale->insert(*m_sliders->at(i),false);

	scale->update();

	return scale;
}

void ccColorScaleEditorWidget::showLabels(bool state)
{
	if (m_labelsWidget)
	{
		m_labelsWidget->setVisible(state);
		m_labelsWidget->update();
	}
}

void ccColorScaleEditorWidget::setLabelColor(QColor color)
{
	if (m_labelsWidget)
	{
		m_labelsWidget->setTextColor(color);
		m_labelsWidget->update();
	}
}

void ccColorScaleEditorWidget::setLabelPrecision(int precision)
{
	if (m_labelsWidget)
	{
		m_labelsWidget->setPrecision(precision);
		m_labelsWidget->update();
	}
}

void ccColorScaleEditorWidget::deleteStep(int index)
{
	assert(m_sliders);

	if (index >= 0)
	{
		if (m_sliders->at(index)->isSelected())
			onSliderSelected(-1);
		m_sliders->removeAt(index);

		update();
	}
}

void ccColorScaleEditorWidget::setStepColor(int index, QColor color)
{
	if (index < 0)
		return;

	m_sliders->at(index)->setColor(color);
	onSliderModified(index);
}

void ccColorScaleEditorWidget::setStepValue(int index, double value)
{
	if (index < 0)
		return;

	m_sliders->at(index)->setValue(value);

	if (m_slidersWidget)
	{
		if (index == 0 || index+1 == m_sliders->size())
		{
			//update all sliders!
			m_slidersWidget->updateAllSlidersPos();
		}
		else
		{
			//update only the selected one
			m_slidersWidget->updateSliderPos(index);
		}
	}

	onSliderModified(index);
}

