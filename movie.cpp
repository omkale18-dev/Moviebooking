// main.cpp
#define CROW_MAIN
#include "crow_all.h"
#include <vector>
#include <string>

using namespace std;

class Movie {
    int id;
    string name;
    string genre;
    int duration;
public:
    Movie(int id, string name, string genre, int duration)
      : id(id), name(name), genre(genre), duration(duration) {}
    int getId() const { return id; }
    string getName() const { return name; }
    string getGenre() const { return genre; }
    int getDuration() const { return duration; }
};

class Show {
    int id;
    Movie movie;
    string time;
    vector<bool> seats; // false=available, true=booked
    int totalSeats;
public:
    Show(int id, Movie movie, string time, int totalSeats=20)
      : id(id), movie(movie), time(time), totalSeats(totalSeats), seats(totalSeats,false) {}

    int getId() const { return id; }
    const Movie& getMovie() const { return movie; }
    string getTime() const { return time; }

    bool isSeatAvailable(int seatNum) const {
        if(seatNum < 1 || seatNum > totalSeats) return false;
        return !seats[seatNum-1];
    }

    bool bookSeat(int seatNum) {
        if(!isSeatAvailable(seatNum)) return false;
        seats[seatNum-1] = true;
        return true;
    }

    vector<int> getAvailableSeats() const {
        vector<int> avail;
        for(int i=0; i<totalSeats; i++)
            if(!seats[i]) avail.push_back(i+1);
        return avail;
    }
};

class BookingSystem {
    vector<Movie> movies;
    vector<Show> shows;

public:
    BookingSystem() {
        // Init movies
        movies.emplace_back(1, "The Matrix", "Sci-Fi", 136);
        movies.emplace_back(2, "Inception", "Thriller", 148);
        movies.emplace_back(3, "The Lion King", "Animation", 88);

        // Init shows
        shows.emplace_back(1, movies[0], "12:00 PM");
        shows.emplace_back(2, movies[0], "6:00 PM");
        shows.emplace_back(3, movies[1], "3:00 PM");
        shows.emplace_back(4, movies[2], "5:00 PM");
    }

    const vector<Movie>& getMovies() const { return movies; }
    const vector<Show>& getShows() const { return shows; }

    Show* getShowById(int id) {
        for(auto& show : shows)
            if(show.getId() == id)
                return &show;
        return nullptr;
    }

    bool bookSeats(int showId, const vector<int>& seats) {
        Show* show = getShowById(showId);
        if(!show) return false;
        for(auto seat : seats) {
            if(!show->isSeatAvailable(seat))
                return false;
        }
        for(auto seat : seats) {
            show->bookSeat(seat);
        }
        return true;
    }
};

int main() {
    crow::SimpleApp app;
    BookingSystem system;

    // Get movies
    CROW_ROUTE(app, "/api/movies")
    ([&system]() {
        crow::json::wvalue res;
        res["movies"] = crow::json::wvalue::array();
        for (const auto& m : system.getMovies()) {
            crow::json::wvalue movieJson;
            movieJson["id"] = m.getId();
            movieJson["name"] = m.getName();
            movieJson["genre"] = m.getGenre();
            movieJson["duration"] = m.getDuration();
            res["movies"].push_back(movieJson);
        }
        return res;
    });

    // Get shows
    CROW_ROUTE(app, "/api/shows")
    ([&system]() {
        crow::json::wvalue res;
        res["shows"] = crow::json::wvalue::array();
        for (const auto& s : system.getShows()) {
            crow::json::wvalue showJson;
            showJson["id"] = s.getId();
            showJson["movieId"] = s.getMovie().getId();
            showJson["movieName"] = s.getMovie().getName();
            showJson["time"] = s.getTime();
            showJson["availableSeats"] = crow::json::wvalue::array();
            auto availSeats = s.getAvailableSeats();
            for(auto seat : availSeats)
                showJson["availableSeats"].push_back(seat);
            res["shows"].push_back(showJson);
        }
        return res;
    });

    // Book seats - POST /api/book with JSON {"showId": int, "seats": [ints]}
    CROW_ROUTE(app, "/api/book").methods("POST"_method)
    ([&system](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body)
            return crow::response(400, "Invalid JSON");

        if (!body.has("showId") || !body.has("seats"))
            return crow::response(400, "Missing parameters");

        int showId = body["showId"].i();
        vector<int> seats;
        for (auto& seat : body["seats"]) {
            if (!seat.is_number())
                return crow::response(400, "Seat numbers must be numeric");
            seats.push_back(seat.i());
        }

        bool success = system.bookSeats(showId, seats);
        crow::json::wvalue res;
        res["success"] = success;
        if (success)
            res["message"] = "Booking successful";
        else
            res["message"] = "Booking failed: seats unavailable or invalid show";

        return crow::response{res};
    });

    app.port(18080).multithreaded().run();
}
