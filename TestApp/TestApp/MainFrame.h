#pragma once
#include <wx/wx.h>
#include <opencv2/opencv.hpp>
#include <wx/timer.h>
#include <wx/thread.h>

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString& title);
    ~MainFrame();

private:
    // Boutons
    wxButton* m_captureButton;
    wxButton* m_snapshotButton;
    wxButton* m_xProfileButton;

    wxStaticText* m_captureText;
    wxStaticText* m_snapshotText;
    wxStaticText* m_xProfileText;

    // Éléments pour la vidéo
    wxPanel* m_videoPanel;
    wxTimer* m_timer;
    cv::VideoCapture m_capture;
    bool m_isCapturing;

    // Pour stocker la frame courante
    cv::Mat m_currentFrame;
    wxMutex m_frameMutex;

    // Méthodes
    void OnTimer(wxTimerEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnCaptureButton(wxCommandEvent& event);
    void OnSnapshotButton(wxCommandEvent& event);
    void OnXProfileButton(wxCommandEvent& event);

    // Convertir une image OpenCV en wxImage
    wxImage ConvertOpenCVToWxImage(const cv::Mat& image);

    DECLARE_EVENT_TABLE()
};