#include <wx/wx.h>

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size)
    {
        // Create a panel
        wxPanel* panel = new wxPanel(this, wxID_ANY);

        // Create a button
        wxButton* button = new wxButton(panel, wxID_ANY, "Click me!", wxPoint(10, 10), wxSize(150, 30));

        // Bind a function to handle button click event
        button->Bind(wxEVT_BUTTON, &MyFrame::OnButtonClick, this);
    }

    void OnButtonClick(wxCommandEvent& event)
    {
        wxMessageBox("Button clicked!", "Info", wxOK | wxICON_INFORMATION);
    }

private:
    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_BUTTON(wxID_ANY, MyFrame::OnButtonClick)
wxEND_EVENT_TABLE()

class MyApp : public wxApp
{
public:
    virtual bool OnInit()
    {
        MyFrame* frame = new MyFrame("wxWidgets Sample", wxPoint(50, 50), wxSize(400, 300));
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
